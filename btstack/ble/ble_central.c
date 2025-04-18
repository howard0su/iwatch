/*
 * Copyright (C) 2011-2013 by BlueKitchen GmbH
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 4. This software may not be used in a commercial product
 *    without an explicit license granted by the copyright holder. 
 *
 * THIS SOFTWARE IS PROVIDED BY MATTHIAS RINGWALD AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MATTHIAS
 * RINGWALD OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

//*****************************************************************************
//
// BLE Client
//
//*****************************************************************************


// NOTE: Supports only a single connection

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <btstack/run_loop.h>
#include <btstack/hci_cmds.h>
#include <btstack/utils.h>
#include <btstack/sdp_util.h>

#include "config.h"

#include "ble_central.h"
#include "ad_parser.h"

#include "debug.h"
#include "btstack_memory.h"
#include "hci.h"
#include "l2cap.h"
#include "att.h"
#include "sm.h"

// gatt client state
typedef enum {
    W4_ON,
    IDLE,
    //
    START_SCAN,
    W4_SCANNING,
    //
    SCANNING,
    //
    STOP_SCAN,
    W4_SCAN_STOPPED
} state_t;


static state_t state = W4_ON;
static linked_list_t le_connections = NULL;
static uint16_t att_client_start_handle = 0x0001;
    

void le_central_init(){
    state = W4_ON;
    le_connections = NULL;
    att_client_start_handle = 0x0000;
}

void (*le_central_callback)(le_central_event_t * event);
static void gatt_client_run();

void dump_nodes(le_peripheral_t * peripheral){
    printf("current level %u, start %02x, end %02x, last %02x\n", peripheral->depth, 
        peripheral->nodes[peripheral->depth].start_group_handle, 
        peripheral->nodes[peripheral->depth].end_group_handle,
        peripheral->nodes[peripheral->depth].last_group_handle);
}

// START Helper Functions - to be sorted
static int l2cap_can_send_conectionless_packet_now(){
    return hci_can_send_packet_now(HCI_ACL_DATA_PACKET);
}

static uint16_t l2cap_max_mtu_for_handle(uint16_t handle){
    return l2cap_max_mtu();
}
// END Helper Functions

static void dummy_notify(le_central_event_t* event){}

void le_central_register_handler(void (*le_callback)(le_central_event_t* event)){
    le_central_callback = dummy_notify;
    if (le_callback != NULL){
        le_central_callback = le_callback;
    } 
}

static inline le_command_status_t send_gatt_request(uint16_t request_type, uint16_t attribute_group_type, uint16_t next_handle, uint16_t end_handle, uint16_t peripheral_handle){
    if (!l2cap_can_send_conectionless_packet_now()) return BLE_PERIPHERAL_BUSY;
    
    uint8_t request[7];
    request[0] = request_type;
    bt_store_16(request, 1, next_handle);
    bt_store_16(request, 3, end_handle);
    bt_store_16(request, 5, attribute_group_type);

    l2cap_send_connectionless(peripheral_handle, L2CAP_CID_ATTRIBUTE_PROTOCOL, request, sizeof(request));
    return BLE_PERIPHERAL_OK;
}

static le_command_status_t send_gatt_include_service_request(le_peripheral_t *peripheral){
    uint16_t next_handle = peripheral->nodes[peripheral->depth].last_group_handle+1;
    uint16_t end_handle  = peripheral->nodes[peripheral->depth].end_group_handle;
    uint16_t peripheral_handle = peripheral->handle;

    return send_gatt_request(ATT_READ_BY_TYPE_REQUEST, GATT_INCLUDE_SERVICE_UUID, next_handle, end_handle, peripheral_handle);
}

static inline void send_gatt_complete_event(le_peripheral_t * peripheral, uint8_t type){
    le_query_complete_event_t event;
    event.type = type;
    event.peripheral = peripheral;
    (*le_central_callback)((le_central_event_t*)&event); 
}


static le_command_status_t send_gatt_characteristic_request(le_peripheral_t *peripheral){
    uint16_t next_handle = peripheral->nodes[peripheral->depth].last_group_handle+1;
    uint16_t end_handle  = peripheral->nodes[peripheral->depth].end_group_handle;
    uint16_t peripheral_handle = peripheral->handle;

    return send_gatt_request(ATT_READ_BY_TYPE_REQUEST, GATT_CHARACTERISTICS_UUID, next_handle, end_handle, peripheral_handle);
}

static le_command_status_t send_gatt_services_request(le_peripheral_t *peripheral){
    uint16_t next_handle = peripheral->nodes[peripheral->depth].last_group_handle+1;
    uint16_t end_handle = 0xffff;
    uint16_t peripheral_handle = peripheral->handle;

    return send_gatt_request(ATT_READ_BY_GROUP_TYPE_REQUEST, GATT_PRIMARY_SERVICE_UUID, next_handle, end_handle, peripheral_handle);
}


static void send_gatt_connection_complete_event(le_peripheral_t * peripheral, uint8_t status){
    le_peripheral_event_t event;
    event.type = GATT_CONNECTION_COMPLETE;
    event.device = peripheral;
    event.status = status;
    (*le_central_callback)((le_central_event_t*)&event); 
}


static le_peripheral_t * get_peripheral_for_handle(uint16_t handle){
    linked_item_t *it;
    for (it = (linked_item_t *) le_connections; it ; it = it->next){
        le_peripheral_t * peripheral = (le_peripheral_t *) it;
        if (peripheral->handle == handle){
            return peripheral;
        }
    }
    return NULL;
}

static le_peripheral_t * get_peripheral_with_state(peripheral_state_t p_state){
    linked_item_t *it;
    for (it = (linked_item_t *) le_connections; it ; it = it->next){
        le_peripheral_t * peripheral = (le_peripheral_t *) it;
        if (peripheral->state == p_state){
            return peripheral;
        }
    }
    return NULL;
}

static inline le_peripheral_t * get_peripheral_w4_disconnected(){
    return get_peripheral_with_state(P_W4_DISCONNECTED);
}

static inline le_peripheral_t * get_peripheral_w4_connect_cancelled(){
    return get_peripheral_with_state(P_W4_CONNECT_CANCELLED);
}

static inline le_peripheral_t * get_peripheral_w2_connect(){
    return get_peripheral_with_state(P_W2_CONNECT);
}

static inline le_peripheral_t * get_peripheral_w4_connected(){
    return get_peripheral_with_state(P_W4_CONNECTED);
}

static inline le_peripheral_t * get_peripheral_w2_exchange_MTU(){
    return get_peripheral_with_state(P_W2_EXCHANGE_MTU);
}

static le_peripheral_t * get_peripheral_with_address(uint8_t addr_type, bd_addr_t addr){
    linked_item_t *it;
    for (it = (linked_item_t *) le_connections; it ; it = it->next){
        le_peripheral_t * peripheral = (le_peripheral_t *) it;
        if (BD_ADDR_CMP(addr, peripheral->address) == 0 && peripheral->address_type == addr_type){
            return peripheral;
        }
    }
    return 0;
}

static void handle_advertising_packet(uint8_t *packet){
    int num_reports = packet[3];
    int i;
    int total_data_length = 0;
    int data_offset = 0;

    for (i=0; i<num_reports;i++){
        total_data_length += packet[4+num_reports*8+i];  
    }

    for (i=0; i<num_reports;i++){
        ad_event_t advertisement_event;
        advertisement_event.type = GATT_ADVERTISEMENT;
        advertisement_event.event_type = packet[4+i];
        advertisement_event.address_type = packet[4+num_reports+i];
        bt_flip_addr(advertisement_event.address, &packet[4+num_reports*2+i*6]);
        advertisement_event.length = packet[4+num_reports*8+i];
        advertisement_event.data = &packet[4+num_reports*9+data_offset];
        data_offset += advertisement_event.length;
        advertisement_event.rssi = packet[4+num_reports*9+total_data_length + i];
        
        (*le_central_callback)((le_central_event_t*)&advertisement_event); 
    }
}

static void handle_peripheral_list(){
    // printf("handle_peripheral_list 1\n");
    // only one connect is allowed, wait for result
    if (get_peripheral_w4_connected()) return;
    // printf("handle_peripheral_list 2\n");
    // only one cancel connect is allowed, wait for result
    if (get_peripheral_w4_connect_cancelled()) return;
    // printf("handle_peripheral_list 3\n");

    if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;
    // printf("handle_peripheral_list 4\n");
    if (!l2cap_can_send_conectionless_packet_now()) return;
    // printf("handle_peripheral_list 5\n");
    
    // printf("handle_peripheral_list empty %u\n", linked_list_empty(&le_connections));
    le_command_status_t status;
    linked_item_t *it;
    for (it = (linked_item_t *) le_connections; it ; it = it->next){
        le_peripheral_t * peripheral = (le_peripheral_t *) it;
        // printf("handle_peripheral_list, status %u\n", peripheral->state);
        switch (peripheral->state){
            case P_W2_CONNECT:
                peripheral->state = P_W4_CONNECTED;
                hci_send_cmd(&hci_le_create_connection, 
                     1000,      // scan interval: 625 ms
                     1000,      // scan interval: 625 ms
                     0,         // don't use whitelist
                     0,         // peer address type: public
                     peripheral->address,      // remote bd addr
                     peripheral->address_type, // random or public
                     80,        // conn interval min
                     80,        // conn interval max (3200 * 0.625)
                     0,         // conn latency
                     2000,      // supervision timeout
                     0,         // min ce length
                     1000       // max ce length
                );
                return;

            case P_W2_CANCEL_CONNECT: 
                peripheral->state = P_W4_CONNECT_CANCELLED;
                hci_send_cmd(&hci_le_create_connection_cancel);
                return;
            case P_W2_EXCHANGE_MTU:
            {
                peripheral->state = P_W4_EXCHANGE_MTU;
                uint16_t mtu = l2cap_max_mtu_for_handle(peripheral->handle);
                uint8_t request[3];
                request[0] = ATT_EXCHANGE_MTU_REQUEST;
                bt_store_16(request, 1, mtu);
                l2cap_send_connectionless(peripheral->handle, L2CAP_CID_ATTRIBUTE_PROTOCOL, request, sizeof(request));
                return;
            }
            case P_W2_SEND_SERVICE_QUERY:
                status = send_gatt_services_request(peripheral);
                if (status != BLE_PERIPHERAL_OK) break;

                peripheral->state = P_W4_SERVICE_QUERY_RESULT;
                break;
            case P_W2_SEND_CHARACTERISTIC_QUERY:
                printf("send characteristic query, level %d\n", peripheral->depth);
                dump_nodes(peripheral);

                status = send_gatt_characteristic_request(peripheral);
                if (status != BLE_PERIPHERAL_OK) break;

                peripheral->state = P_W4_CHARACTERISTIC_QUERY_RESULT;
                break;
            case P_W2_SEND_INCLUDE_SERVICE_QUERY:
                printf("send include query\n"); 
                dump_nodes(peripheral);
                status = send_gatt_include_service_request(peripheral);
                if (status != BLE_PERIPHERAL_OK) break;

                peripheral->state = P_W4_INCLUDE_SERVICE_QUERY_RESULT;
                break;
            case P_W2_DISCONNECT:
                peripheral->state = P_W4_DISCONNECTED;
                hci_send_cmd(&hci_disconnect, peripheral->handle,0x13);
                return;

            default:
                break;
        }
        
    }
         
}


le_command_status_t le_central_start_scan(){
    if (state != IDLE) return BLE_PERIPHERAL_IN_WRONG_STATE; 
    state = START_SCAN;
    gatt_client_run();
    return BLE_PERIPHERAL_OK;
}

le_command_status_t le_central_stop_scan(){
    if (state != SCANNING) return BLE_PERIPHERAL_IN_WRONG_STATE;
    state = STOP_SCAN;
    gatt_client_run();
    return BLE_PERIPHERAL_OK;
}

static void le_peripheral_init(le_peripheral_t *context, uint8_t addr_type, bd_addr_t addr){
    memset(context, 0, sizeof(le_peripheral_t));
    context->state = P_W2_CONNECT;
    context->address_type = addr_type;
    memcpy (context->address, addr, 6);
}

le_command_status_t le_central_connect(le_peripheral_t *context, uint8_t addr_type, bd_addr_t addr){
    //TODO: align with hci connection list capacity
    le_peripheral_t * peripheral = get_peripheral_with_address(addr_type, addr);
    if (!peripheral) {
        printf("le_central_connect: new context, init\n");
        le_peripheral_init(context, addr_type, addr);
        linked_list_add(&le_connections, (linked_item_t *) context);
    } else if (peripheral == context) {
        if (context->state != P_W2_CONNECT) return BLE_PERIPHERAL_IN_WRONG_STATE;
    } else {
        return BLE_PERIPHERAL_DIFFERENT_CONTEXT_FOR_ADDRESS_ALREADY_EXISTS;
    }

    printf("le_central_connect: connections is empty %u\n", linked_list_empty(&le_connections));

    gatt_client_run();
    return BLE_PERIPHERAL_OK;
}

le_command_status_t le_central_disconnect(le_peripheral_t *context){
    // printf("*** le_central_disconnect::CALLED DISCONNECT \n");

    le_peripheral_t * peripheral = get_peripheral_with_address(context->address_type, context->address);
    if (!peripheral || (peripheral && peripheral != context)){
        return BLE_PERIPHERAL_DIFFERENT_CONTEXT_FOR_ADDRESS_ALREADY_EXISTS;
    }

    switch(context->state){
        case P_W2_CONNECT:
            linked_list_remove(&le_connections, (linked_item_t *) context);
            send_gatt_connection_complete_event(context, 0);
            break; 
        case P_W4_CONNECTED:
        case P_W2_CANCEL_CONNECT:
            // trigger cancel connect
            context->state = P_W2_CANCEL_CONNECT;
            break;
        case P_W4_DISCONNECTED:
        case P_W4_CONNECT_CANCELLED: 
            return BLE_PERIPHERAL_IN_WRONG_STATE;
        default:
            context->state = P_W2_DISCONNECT;
            break;
    }  
    gatt_client_run();    
    return BLE_PERIPHERAL_OK;      
}

le_command_status_t le_central_get_services(le_peripheral_t *peripheral){
    if (peripheral->state != P_CONNECTED) return BLE_PERIPHERAL_IN_WRONG_STATE;
    // printf("le_central_get_services ready to send\n");
    peripheral->nodes[0].last_group_handle = 0x0000;
    peripheral->depth = 0;
    peripheral->state = P_W2_SEND_SERVICE_QUERY;
    gatt_client_run();
    return BLE_PERIPHERAL_OK;
}

le_command_status_t le_central_get_characteristics_for_service(le_peripheral_t *peripheral, le_service_t *service){
    if (peripheral->state != P_CONNECTED) return BLE_PERIPHERAL_IN_WRONG_STATE;
    printf("le_central_get_characteristics_for_service ready to send\n");
    
    // send will increment last_group_handle, so substract in advance
    peripheral->depth = 0;
    peripheral->nodes[peripheral->depth].start_group_handle = service->start_group_handle;
    peripheral->nodes[peripheral->depth].end_group_handle = service->end_group_handle;
    peripheral->nodes[peripheral->depth].last_group_handle = service->start_group_handle - 1;

    peripheral->state = P_W2_SEND_CHARACTERISTIC_QUERY;
    gatt_client_run();
    return BLE_PERIPHERAL_OK;
}

extern void test_client();

static void gatt_client_run(){
    if (state == W4_ON) return;
    
    handle_peripheral_list();

    // check if command is send 
    if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;
    
    switch(state){
        case START_SCAN:
            state = W4_SCANNING;
            hci_send_cmd(&hci_le_set_scan_enable, 1, 0);
            return;

        case STOP_SCAN:
            state = W4_SCAN_STOPPED;
            hci_send_cmd(&hci_le_set_scan_enable, 0, 0);
            return;

        default:
            break;
    }

    test_client();
}


static void packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size){

    if (packet_type != HCI_EVENT_PACKET) return;
    
	switch (packet[0]) {
		case BTSTACK_EVENT_STATE:
			// BTstack activated, get started
			if (packet[2] == HCI_STATE_WORKING) {
                printf("BTstack activated, get started!\n");
                state = IDLE;
            }
			break;

        case HCI_EVENT_COMMAND_COMPLETE:
            if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_scan_enable)){
                switch(state){
                    case W4_SCANNING:
                        state = SCANNING;
                        break;
                    case W4_SCAN_STOPPED:
                        state = IDLE;
                        break;
                    default:
                        break;
                }
                break;
            }

            if (COMMAND_COMPLETE_EVENT(packet, hci_le_create_connection_cancel)){
                // printf("packet_handler:: hci_le_create_connection_cancel: cancel connect\n");
                if (packet[3] != 0x0B) break;

                // cancel connection failed, as connection already established
                le_peripheral_t * peripheral = get_peripheral_w4_connect_cancelled();
                peripheral->state = P_W2_DISCONNECT;
                break;
            }
            break;  

        case HCI_EVENT_DISCONNECTION_COMPLETE:
        {
            uint16_t handle = READ_BT_16(packet,3);
            le_peripheral_t * peripheral = get_peripheral_for_handle(handle);
            if (!peripheral) break;

            peripheral->state = P_W2_CONNECT;
            linked_list_remove(&le_connections, (linked_item_t *) peripheral);
            send_gatt_connection_complete_event(peripheral, packet[5]);
            // printf("Peripheral disconnected, and removed from list\n");
            break;
        }

        case HCI_EVENT_LE_META:
            switch (packet[2]) {
                case HCI_SUBEVENT_LE_ADVERTISING_REPORT: 
                    if (state != SCANNING) break;
                    handle_advertising_packet(packet);
                    break;
                
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                    // deal with conn cancel, conn fail, conn success
                    le_peripheral_t * peripheral;

                    // conn success/error?
                    peripheral = get_peripheral_w4_connected();
                    if (peripheral){
                        if (packet[3]){
                            // error
                            linked_list_remove(&le_connections, (linked_item_t *) peripheral);
                        } else {
                            // success
                            peripheral->state = P_W2_EXCHANGE_MTU;
                            peripheral->handle = READ_BT_16(packet, 4);
                        }
                        send_gatt_connection_complete_event(peripheral, packet[3]);
                        break;
                    } 

                    // cancel success?
                    peripheral = get_peripheral_w4_connect_cancelled();
                    if (!peripheral) break;
                            
                    linked_list_remove(&le_connections, (linked_item_t *) peripheral);
                    send_gatt_connection_complete_event(peripheral, packet[3]);
                    break;
                }
                default:
                    break;
            }
            break;
        
        default:
            break;
    }
    gatt_client_run();
}

static const char * att_errors[] = {
    "OK",
    "Invalid Handle",
    "Read Not Permitted",
    "Write Not Permitted",
    "Invalid PDU",
    "Insufficient Authentication",
    "Request Not Supported",
    "Invalid Offset",
    "Insufficient Authorization",
    "Prepare Queue Full",
    "Attribute Not Found",
    "Attribute Not Long",
    "Insufficient Encryption Key Size",
    "Invalid Attribute Value Length",
    "Unlikely Error",
    "Insufficient Encryption",
    "Unsupported Group Type",
    "Insufficient Resources"
};

static void att_client_report_error(uint8_t * packet, uint16_t size){
    
    uint8_t error_code = packet[4];
    const char * error = "Unknown";
    if (error_code <= 0x11){
        error = att_errors[error_code];
    }
    uint16_t handle = READ_BT_16(packet, 2);
    printf("ATT_ERROR_REPORT handle 0x%04x, error: %u - %s\n", handle, error_code, error);
}


static void handle_gatt_service_packet(le_peripheral_t * peripheral, uint8_t * packet, uint16_t uuid_length){
    le_service_event_t event;
    le_service_t service;

    event.type = GATT_SERVICE_QUERY_RESULT;
    service.start_group_handle = READ_BT_16(packet,0);
    service.end_group_handle = READ_BT_16(packet,2);
    
    if (uuid_length == 2){
        service.uuid16 = READ_BT_16(packet, 4);
        sdp_normalize_uuid((uint8_t*) &service.uuid128, service.uuid16);
    } else {
        memcpy(service.uuid128, &packet[4], uuid_length);
    }
    event.service = service;
    (*le_central_callback)((le_central_event_t*)&event);
    
    if (peripheral->nodes[0].last_group_handle < service.end_group_handle){
        peripheral->nodes[0].last_group_handle = service.end_group_handle;
    }
}



int has_more_handles_on_current_level(le_peripheral_t * peripheral){
    return peripheral->nodes[peripheral->depth].last_group_handle < peripheral->nodes[peripheral->depth].end_group_handle;
}


int find_next_handle(le_peripheral_t * peripheral){
    
    printf("find next node: ");
    dump_nodes(peripheral);

    if (has_more_handles_on_current_level(peripheral)){
        peripheral->nodes[peripheral->depth].last_group_handle++;
        printf("found on the same level : ");
        dump_nodes(peripheral);
        return 1;
    }

    if (peripheral->depth < LE_CENTRAL_MAX_INCLUDE_DEPTH){
        printf("go deeper: ");
        dump_nodes(peripheral);
        peripheral->depth++;
    } else if (peripheral->depth == 0){
        printf("no more nodes\n");
        return 0;
    } else { 
        printf("go up: ");
        dump_nodes(peripheral);
        peripheral->depth--;
    }
    return find_next_handle(peripheral);
}


static void att_packet_handler(uint8_t packet_type, uint16_t handle, uint8_t *packet, uint16_t size){
    if (packet_type != ATT_DATA_PACKET) return;
    le_peripheral_t * peripheral = get_peripheral_for_handle(handle);
    if (!peripheral) return;

    switch (packet[0]){
        case ATT_EXCHANGE_MTU_RESPONSE:
        {
            uint16_t remote_rx_mtu = READ_BT_16(packet, 1);
            uint16_t local_rx_mtu = l2cap_max_mtu_for_handle(handle);
            peripheral->mtu = remote_rx_mtu < local_rx_mtu ? remote_rx_mtu : local_rx_mtu;

            send_gatt_connection_complete_event(peripheral, 0);
            
            peripheral->state = P_CONNECTED; 
            break;
        }
        case ATT_READ_BY_GROUP_TYPE_RESPONSE:
        // if (peripheral->state != P_W4_SERVICE_QUERY_RESULT) return;
        {
            printf("att_packet_handler :: ATT_READ_BY_GROUP_TYPE_RESPONSE\n");
        
            uint8_t attr_length = packet[1];
            uint8_t uuid_length = attr_length - 4;

            int i;
            for (i = 2; i < size; i += attr_length){
                handle_gatt_service_packet(peripheral, &packet[i], uuid_length);
            }

            if (peripheral->nodes[0].last_group_handle < 0xffff){
                peripheral->state = P_W2_SEND_SERVICE_QUERY;
                break;
            }
            printf("att_packet_handler :: ATT_READ_BY_GROUP_TYPE_RESPONSE ---> DONE\n");
            // DONE
            peripheral->state = P_CONNECTED;
            send_gatt_complete_event(peripheral, GATT_SERVICE_QUERY_COMPLETE);

            break;
        }
        case ATT_READ_BY_TYPE_RESPONSE:
        {
            uint8_t attr_length = packet[1];
            uint8_t uuid_length;
            int i;

            switch (peripheral->state){
                case P_W4_CHARACTERISTIC_QUERY_RESULT:{
                    uuid_length = attr_length - 5;
                    
                    for (i = 2; i < size; i += attr_length){
                        le_characteristic_event_t event;
                        le_characteristic_t characteristic;

                        event.type = GATT_CHARACTERISTIC_QUERY_RESULT;

                        peripheral->nodes[peripheral->depth].last_group_handle = READ_BT_16(packet,i);

                        characteristic.properties = packet[i+2];
                        characteristic.value_handle = READ_BT_16(packet, i+3);

                        if (uuid_length == 2){
                            characteristic.uuid16 = READ_BT_16(packet,i+5);
                            sdp_normalize_uuid((uint8_t*) &characteristic.uuid128, characteristic.uuid16);
                        } else {
                            memcpy(characteristic.uuid128, &packet[i+5], uuid_length);
                        }

                        event.characteristic = characteristic;
                        (*le_central_callback)((le_central_event_t*)&event);
                    }

                    // query CHR on the same level
                    if (has_more_handles_on_current_level(peripheral)){
                        printf("more CHR on level\n");
                        peripheral->state = P_W2_SEND_CHARACTERISTIC_QUERY;
                        peripheral->nodes[peripheral->depth].last_group_handle++;
                        break;
                    } 

                    // start query includes on the same level
                    // do not ask for include on the max level...
                    if (peripheral->depth + 1 < LE_CENTRAL_MAX_INCLUDE_DEPTH){
                        peripheral->nodes[peripheral->depth].last_group_handle = peripheral->nodes[peripheral->depth].start_group_handle - 1;
                        peripheral->state = P_W2_SEND_INCLUDE_SERVICE_QUERY;
                        break;
                    } 
                    
                    // no includes on the current level -> go up
                    while (peripheral->depth > 0 && !has_more_handles_on_current_level(peripheral)){
                        peripheral->depth--;
                    }

                    if (has_more_handles_on_current_level(peripheral)){
                        peripheral->nodes[peripheral->depth].last_group_handle++;
                        peripheral->state = P_W2_SEND_INCLUDE_SERVICE_QUERY;
                        break;
                    }
                    peripheral->state = P_CONNECTED;
                    send_gatt_complete_event(peripheral, GATT_CHARACTERISTIC_QUERY_COMPLETE); 
                
                    break;
                }

                case P_W4_INCLUDE_SERVICE_QUERY_RESULT:{
                    peripheral->nodes[peripheral->depth].last_group_handle = READ_BT_16(packet,2);
                    
                    // update handle 
                    printf("go down one level, ask for CHR\n");
                    peripheral->depth++;
                    peripheral->nodes[peripheral->depth].start_group_handle = READ_BT_16(packet,4);
                    peripheral->nodes[peripheral->depth].end_group_handle = READ_BT_16(packet,6);
                    peripheral->nodes[peripheral->depth].last_group_handle = peripheral->nodes[peripheral->depth].start_group_handle - 1;
                    
                    peripheral->state = P_W2_SEND_CHARACTERISTIC_QUERY;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case ATT_ERROR_RESPONSE:
            printf("ATT_ERROR_RESPONSE error %u, state %u\n", packet[4], peripheral->state);
            switch (packet[4]){
                case ATT_ERROR_ATTRIBUTE_NOT_FOUND: {
                    switch(peripheral->state){
                        case P_W4_SERVICE_QUERY_RESULT:
                            peripheral->state = P_CONNECTED;
                            send_gatt_complete_event(peripheral, GATT_SERVICE_QUERY_COMPLETE);
                            break;
                        case P_W4_CHARACTERISTIC_QUERY_RESULT:
                            printf("CHARACTERISTIC_QUERY ERROR search includes\n");
                            
                            // start query includes on the same level
                            // do not ask for include on the max level...
                            if (peripheral->depth + 1 < LE_CENTRAL_MAX_INCLUDE_DEPTH){
                                peripheral->nodes[peripheral->depth].last_group_handle = peripheral->nodes[peripheral->depth].start_group_handle - 1;
                                peripheral->state = P_W2_SEND_INCLUDE_SERVICE_QUERY;
                                break;
                            } 
                            
                            // no includes on the current level -> go up
                            while (peripheral->depth > 0 && !has_more_handles_on_current_level(peripheral)){
                                peripheral->depth--;
                            }

                            if (has_more_handles_on_current_level(peripheral)){
                                peripheral->nodes[peripheral->depth].last_group_handle++;
                                peripheral->state = P_W2_SEND_INCLUDE_SERVICE_QUERY;
                                break;
                            }
                            peripheral->state = P_CONNECTED;
                            send_gatt_complete_event(peripheral, GATT_CHARACTERISTIC_QUERY_COMPLETE);


                            break;

                        case P_W4_INCLUDE_SERVICE_QUERY_RESULT:
                            printf("INCLUDE_SERVICE_QUERY ERROR\n");
                            
                            // no includes on the current level -> go up
                            while (peripheral->depth > 0 && !has_more_handles_on_current_level(peripheral)){
                                peripheral->depth--;
                            }

                            if (has_more_handles_on_current_level(peripheral)){
                                peripheral->nodes[peripheral->depth].last_group_handle++;
                                peripheral->state = P_W2_SEND_INCLUDE_SERVICE_QUERY;
                                break;
                            }
                            peripheral->state = P_CONNECTED;
                            send_gatt_complete_event(peripheral, GATT_CHARACTERISTIC_QUERY_COMPLETE);


                            break;
                        default:
                            return;
                    }
                    break;
                }
                default:                
                    att_client_report_error(packet, size);
                    break;
            }
            break;

        default:
            printf("ATT Handler, unhandled response type 0x%02x\n", packet[0]);
            break;
    }
    gatt_client_run();
}

static void dump_state(){
    switch(state){
        case W4_ON: printf("W4_ON"); break;
        case IDLE: printf("IDLE"); break;
        case START_SCAN: printf("START_SCAN"); break;
        case W4_SCANNING: printf("W4_SCANNING"); break;
        case SCANNING: printf("SCANNING"); break;
        case STOP_SCAN: printf("STOP_SCAN"); break;
        case W4_SCAN_STOPPED: printf("W4_SCAN_STOPPED"); break;
    };
    printf(" : ");
}

int ble_init(void)
{
    l2cap_register_fixed_channel(att_packet_handler, L2CAP_CID_ATTRIBUTE_PROTOCOL);
    l2cap_register_packet_handler(packet_handler);
 
    le_central_init();
   
    central_device_db_init();

    sm_init();

    sm_set_io_capabilities(3);
    sm_set_authentication_requirements( SM_AUTHREQ_BONDING );
    sm_set_request_security(1);

    // happy compiler!
    return 0;
}





