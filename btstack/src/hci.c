/*
 * Copyright (C) 2009 by Matthias Ringwald
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

/*
 *  hci.c
 *
 *  Created by Matthias Ringwald on 4/29/09.
 *
 */
//#define ENABLE_LOG_INFO
#include "btstack-config.h"
#include "btstack/utils.h"
#include "gap.h"
#include "hci.h"

#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "system.h" // system_getserial

#ifndef EMBEDDED
#include <unistd.h> // gethostbyname
#include <btstack/version.h>
#endif

#include "btstack_memory.h"
#include "debug.h"
#include "hci_dump.h"

#include <btstack/hci_cmds.h>

#define HCI_CONNECTION_TIMEOUT_MS 30000

#define HCI_INTIALIZING_SUBSTATE_AFTER_SLEEP 11

#ifdef USE_BLUETOOL
#include "bt_control_iphone.h"
#endif

static void hci_update_scan_enable(void);
static gap_security_level_t gap_security_level_for_connection(hci_connection_t * connection);

// the STACK is here
#ifndef HAVE_MALLOC
static hci_stack_t   hci_stack_static;
#endif
static hci_stack_t * hci_stack = NULL;

// test helper
static uint8_t disable_l2cap_timeouts = 0;

static const uint16_t       sniff_max_interval = 0x340;  // time unit: 0.625ms
static const uint16_t       sniff_min_interval = 0xA0;
static const uint16_t       sniff_attempt = 1;
static const uint16_t       sniff_timeout = 0x10;

/**
 * get connection for a given handle
 *
 * @return connection OR NULL, if not found
 */
hci_connection_t * hci_connection_for_handle(hci_con_handle_t con_handle){
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        if ( ((hci_connection_t *) it)->con_handle == con_handle){
            return (hci_connection_t *) it;
        }
    }
    return NULL;
}

hci_connection_t * hci_connection_for_bd_addr(bd_addr_t * addr){
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
        if (memcmp(addr, connection->address, 6) == 0) {
            return connection;
        }
    }
    return NULL;
}

static void hci_connection_timeout_handler(timer_source_t *timer){
    hci_connection_t * connection = (hci_connection_t *) linked_item_get_user(&timer->item);
#ifdef HAVE_TIME
    struct timeval tv;
    gettimeofday(&tv, NULL);
    if (tv.tv_sec >= connection->timestamp.tv_sec + HCI_CONNECTION_TIMEOUT_MS/1000) {
        // connections might be timed out
        hci_emit_l2cap_check_timeout(connection);
    }
#endif
#ifdef HAVE_TICK
    if (embedded_get_ticks() > connection->timestamp + embedded_ticks_for_ms(HCI_CONNECTION_TIMEOUT_MS)){
        // connections might be timed out
        hci_emit_l2cap_check_timeout(connection);
    }
    if (connection->sniff_timeout != -1 &&
        (connection->mode & MODE_VALUES) == ACTIVE &&
        embedded_get_ticks() > connection->timestamp + embedded_ticks_for_ms(connection->sniff_timeout)){
        // switch to sniff mode
        log_debug("try to go to sniff mode\n");
        connection->mode |= ENTER_SNIFF;
        hci_run();
    }
#endif
    if (connection->sniff_timeout < HCI_CONNECTION_TIMEOUT_MS && connection->mode == ACTIVE)
        run_loop_set_timer(timer, connection->sniff_timeout);
    else
    run_loop_set_timer(timer, HCI_CONNECTION_TIMEOUT_MS);
    
    run_loop_add_timer(timer);
}

static void hci_connection_timestamp(hci_connection_t *connection){
#ifdef HAVE_TIME
    gettimeofday(&connection->timestamp, NULL);
#endif
#ifdef HAVE_TICK
    connection->timestamp = embedded_get_ticks();
#endif
}

/**
 * create connection for given address
 *
 * @return connection OR NULL, if no memory left
 */
static hci_connection_t * create_connection_for_addr(bd_addr_t addr, uint8_t type){

    printf("create_connection_for_addr %s\n", bd_addr_to_str(addr));
    hci_connection_t * conn = (hci_connection_t *) btstack_memory_hci_connection_get();
    if (!conn) return NULL;
    BD_ADDR_COPY(conn->address, addr);
	conn->type = type;
    conn->mode = ACTIVE;
    conn->con_handle = 0xffff;
    conn->sniff_timeout = 0xffff;
    conn->authentication_flags = AUTH_FLAGS_NONE;
    conn->bonding_flags = 0;
    conn->requested_security_level = LEVEL_0;
    linked_item_set_user(&conn->timeout.item, conn);
    conn->timeout.process = hci_connection_timeout_handler;
    hci_connection_timestamp(conn);
    conn->acl_recombination_length = 0;
    conn->acl_recombination_pos = 0;
    conn->num_acl_packets_sent = 0;
    linked_list_add(&hci_stack->connections, (linked_item_t *) conn);
    return conn;
}

/**
 * get connection for given address
 *
 * @return connection OR NULL, if not found
 */
static hci_connection_t * connection_for_address(bd_addr_t address, uint8_t type){
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        if ( ! BD_ADDR_CMP( ((hci_connection_t *) it)->address, address) && type == ((hci_connection_t *) it)->type){
            return (hci_connection_t *) it;
        }
    }
    return NULL;
}

inline static void connectionSetAuthenticationFlags(hci_connection_t * conn, hci_authentication_flags_t flags){
    conn->authentication_flags = (hci_authentication_flags_t)(conn->authentication_flags | flags);
}

inline static void connectionClearAuthenticationFlags(hci_connection_t * conn, hci_authentication_flags_t flags){
    conn->authentication_flags = (hci_authentication_flags_t)(conn->authentication_flags & ~flags);
}


/**
 * add authentication flags and reset timer
 */
static void hci_add_connection_flags_for_flipped_bd_addr(uint8_t *bd_addr, hci_authentication_flags_t flags){
    bd_addr_t addr;
    bt_flip_addr(addr, *(bd_addr_t *) bd_addr);
    hci_connection_t * conn = connection_for_address(addr, 1); // must be ACL
    if (conn) {
        connectionSetAuthenticationFlags(conn, flags);
        hci_connection_timestamp(conn);
    }
}

int  hci_authentication_active_for_handle(hci_con_handle_t handle){
    hci_connection_t * conn = hci_connection_for_handle(handle);
    if (!conn) return 0;
    if (conn->authentication_flags & LEGACY_PAIRING_ACTIVE) return 1;
    if (conn->authentication_flags & SSP_PAIRING_ACTIVE) return 1;
    return 0;
}

void hci_drop_link_key_for_bd_addr(bd_addr_t *addr){
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->delete_link_key(addr);
    }
}


/**
 * count connections
 */
static int nr_hci_connections(void){
    int count = 0;
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next, count++);
    return count;
}

/** 
 * Dummy handler called by HCI
 */
static void dummy_handler(uint8_t packet_type, uint8_t *packet, uint16_t size){
}

uint8_t hci_number_outgoing_packets(hci_con_handle_t handle){
    hci_connection_t * connection = hci_connection_for_handle(handle);
    if (!connection) {
        log_error("hci_number_outgoing_packets connectino for handle %u does not exist!\n", handle);
        return 0;
    }
    return connection->num_acl_packets_sent;
}

uint8_t hci_number_free_acl_slots(){
    uint8_t free_slots = hci_stack->total_num_acl_packets;
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
        if (connection->con_handle > 1024)
            continue;
        if (free_slots < connection->num_acl_packets_sent) {
            log_error("hci_number_free_acl_slots: sum of outgoing packets > total acl packets!\n");
            return 0;
        }
        free_slots -= connection->num_acl_packets_sent;
    }
    return free_slots;
}

uint8_t hci_number_free_le_slots(){
    uint8_t free_slots = hci_stack->total_num_le_packets;
    linked_item_t *it;
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){
        hci_connection_t * connection = (hci_connection_t *) it;
        if (connection->con_handle <= 1024)
            continue;
        if (free_slots < connection->num_acl_packets_sent) {
            log_error("hci_number_free_le_slots: sum of outgoing packets > total le packets!\n");
            return 0;
        }
        free_slots -= connection->num_acl_packets_sent;
    }
    return free_slots;
}

int hci_can_send_packet_now(uint8_t packet_type){

    // check for async hci transport implementations
    if (hci_stack->hci_transport->can_send_packet_now){
        if (!hci_stack->hci_transport->can_send_packet_now(packet_type)){
            return 0;
        }
    }
    
    // check regular Bluetooth flow control
    switch (packet_type) {
        case HCI_ACL_DATA_PACKET:
            return hci_number_free_acl_slots();
        case HCI_COMMAND_DATA_PACKET:
            return hci_stack->num_cmd_packets;
        case HCI_LE_DATA_PACKET:
            return hci_number_free_le_slots();
        default:
            return 0;
    }
}

int hci_send_acl_packet(uint8_t *packet, int size){

    // check for free places on BT module
    
    hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);

    if (con_handle <= 1024)
        if (!hci_number_free_acl_slots()) return BTSTACK_ACL_BUFFERS_FULL;
    else
        if (!hci_number_free_le_slots()) return BTSTACK_ACL_BUFFERS_FULL;

    hci_connection_t *connection = hci_connection_for_handle( con_handle);
    if (!connection) return 0;
    hci_connection_timestamp(connection);
    
    // count packet
    connection->num_acl_packets_sent++;

    // send packet 
    int err = hci_stack->hci_transport->send_packet(HCI_ACL_DATA_PACKET, packet, size);
    
    log_info("hci_send_acl_packet - handle %u, sent %u error: %u\n", connection->con_handle, connection->num_acl_packets_sent, err);
    hci_dump_packet( HCI_ACL_DATA_PACKET, 0, packet, size);

    return err;
}

static void acl_handler(uint8_t *packet, int size){

    // log_info("acl_handler: size %u", size);

    // get info
    hci_con_handle_t con_handle = READ_ACL_CONNECTION_HANDLE(packet);
    hci_connection_t *conn      = hci_connection_for_handle(con_handle);
    uint8_t  acl_flags          = READ_ACL_FLAGS(packet);
    uint16_t acl_length         = READ_ACL_LENGTH(packet);

    hci_dump_packet( HCI_ACL_DATA_PACKET, 1, packet, size);

    // ignore non-registered handle
    if (!conn){
        log_error( "hci.c: acl_handler called with non-registered handle %u!\n" , con_handle);
        return;
    }

    // assert packet is complete    
    if (acl_length + 4 != size){
        log_error("hci.c: acl_handler called with ACL packet of wrong size %u, expected %u => dropping packet", size, acl_length + 4);
        return;
    }

    // update idle timestamp
    hci_connection_timestamp(conn);
    
    // handle different packet types
    switch (acl_flags & 0x03) {
            
        case 0x01: // continuation fragment
            
            // sanity check
            if (conn->acl_recombination_pos == 0) {
                log_error( "ACL Cont Fragment but no first fragment for handle 0x%02x\n", con_handle);
                return;
            }
            
            // append fragment payload (header already stored)
            memcpy(&conn->acl_recombination_buffer[conn->acl_recombination_pos], &packet[4], acl_length );
            conn->acl_recombination_pos += acl_length;
            
            // log_error( "ACL Cont Fragment: acl_len %u, combined_len %u, l2cap_len %u\n", acl_length,
            //        conn->acl_recombination_pos, conn->acl_recombination_length);  
            
            // forward complete L2CAP packet if complete. 
            if (conn->acl_recombination_pos >= conn->acl_recombination_length + 4 + 4){ // pos already incl. ACL header
                
                hci_stack->packet_handler(HCI_ACL_DATA_PACKET, conn->acl_recombination_buffer, conn->acl_recombination_pos);
                // reset recombination buffer
                conn->acl_recombination_length = 0;
                conn->acl_recombination_pos = 0;
            }
            break;
            
        case 0x02: { // first fragment
            
            // sanity check
            if (conn->acl_recombination_pos) {
                log_error( "ACL First Fragment but data in buffer for handle 0x%02x\n", con_handle);
                return;
            }

            // peek into L2CAP packet!
            uint16_t l2cap_length = READ_L2CAP_LENGTH( packet );

            // log_info( "ACL First Fragment: acl_len %u, l2cap_len %u\n", acl_length, l2cap_length);

            // compare fragment size to L2CAP packet size
            if (acl_length >= l2cap_length + 4){
                
                // forward fragment as L2CAP packet
                hci_stack->packet_handler(HCI_ACL_DATA_PACKET, packet, acl_length + 4);
            
            } else {
                // store first fragment and tweak acl length for complete package
                memcpy(conn->acl_recombination_buffer, packet, acl_length + 4);
                conn->acl_recombination_pos    = acl_length + 4;
                conn->acl_recombination_length = l2cap_length;
                bt_store_16(conn->acl_recombination_buffer, 2, l2cap_length +4);
            }
            break;
            
        } 
        default:
            log_error( "hci.c: acl_handler called with invalid packet boundary flags %u\n", acl_flags & 0x03);
            return;
    }
    
    // execute main loop
    hci_run();
}

static void hci_shutdown_connection(hci_connection_t *conn){
    log_info("Connection closed: handle %u, %s\n", conn->con_handle, bd_addr_to_str(conn->address));

    // cancel all l2cap connections
    hci_emit_disconnection_complete(conn->con_handle, 0x16);    // terminated by local host

    run_loop_remove_timer(&conn->timeout);
    
    linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
    btstack_memory_hci_connection_free( conn );
    
    // now it's gone
    hci_emit_nr_connections_changed();
}

static const uint16_t packet_type_sizes[] = {
    0, HCI_ACL_2DH1_SIZE, HCI_ACL_3DH1_SIZE, HCI_ACL_DM1_SIZE,
    HCI_ACL_DH1_SIZE, 0, 0, 0,
    HCI_ACL_2DH3_SIZE, HCI_ACL_3DH3_SIZE, HCI_ACL_DM3_SIZE, HCI_ACL_DH3_SIZE,
    HCI_ACL_2DH5_SIZE, HCI_ACL_3DH5_SIZE, HCI_ACL_DM5_SIZE, HCI_ACL_DH5_SIZE
};
static const uint8_t  packet_type_feature_requirement_bit[] = {
     0, // 3 slot packets
     1, // 5 slot packets
    25, // EDR 2 mpbs
    26, // EDR 3 mbps
    39, // 3 slot EDR packts
    40, // 5 slot EDR packet
};
static const uint16_t packet_type_feature_packet_mask[] = {
    0x0f00, // 3 slot packets
    0xf000, // 5 slot packets
    0x1102, // EDR 2 mpbs
    0x2204, // EDR 3 mbps
    0x0300, // 3 slot EDR packts
    0x3000, // 5 slot EDR packet
};

static uint16_t hci_acl_packet_types_for_buffer_size_and_local_features(uint16_t buffer_size, uint8_t * local_supported_features){
    // enable packet types based on size
    uint16_t packet_types = 0;
    int i;
    for (i=0;i<16;i++){
        if (packet_type_sizes[i] == 0) continue;
        if (packet_type_sizes[i] <= buffer_size){
            packet_types |= 1 << i;
        }
    }
    // disable packet types due to missing local supported features
    for (i=0;i<sizeof(packet_type_feature_requirement_bit);i++){
        int bit_idx = packet_type_feature_requirement_bit[i];
        int feature_set = (local_supported_features[bit_idx >> 3] & (1<<(bit_idx & 7))) != 0;
        if (feature_set) continue;
        log_info("Features bit %02u is not set, removing packet types 0x%04x", bit_idx, packet_type_feature_packet_mask[i]);
        packet_types &= ~packet_type_feature_packet_mask[i];
    }
    // flip bits for "may not be used"
    packet_types ^= 0x3306;
    return packet_types;
}

uint16_t hci_usable_acl_packet_types(void){
    return hci_stack->packet_types;
}

uint8_t* hci_get_outgoing_acl_packet_buffer(void){
    // hci packet buffer is >= acl data packet length
    return hci_stack->hci_packet_buffer;
}

uint16_t hci_max_acl_data_packet_length(void){
    return hci_stack->acl_data_packet_length;
}

int hci_ssp_supported(void){
    // No 51, byte 6, bit 3
    return (hci_stack->local_supported_features[6] & (1 << 3)) != 0;
}

int hci_classic_supported(void){
    // No 37, byte 4, bit 5, = No BR/EDR Support
    return (hci_stack->local_supported_features[4] & (1 << 5)) == 0;
}

int hci_le_supported(void){
    // No 37, byte 4, bit 6 = LE Supported (Controller)
#ifdef HAVE_BLE
    return (hci_stack->local_supported_features[4] & (1 << 6)) != 0;
#else
    return 0;
#endif    
}

// get addr type and address used in advertisement packets
void hci_le_advertisement_address(uint8_t * addr_type, bd_addr_t * addr){
    *addr_type = hci_stack->adv_addr_type;
    if (hci_stack->adv_addr_type){
        memcpy(addr, hci_stack->adv_address, 6);
    } else {
        memcpy(addr, hci_stack->local_bd_addr, 6);
    }
}

// avoid huge local variables
#ifndef EMBEDDED
static device_name_t device_name;
#endif
static void event_handler(uint8_t *packet, int size){

    uint16_t event_length = packet[1];

    // assert packet is complete    
    if (size != event_length + 2){
        log_error("hci.c: event_handler called with event packet of wrong size %u, expected %u => dropping packet", size, event_length + 2);
        return;
    }

    bd_addr_t addr;
    uint8_t link_type;
    hci_con_handle_t handle;
    hci_connection_t * conn;
    int i;
        
    // printf("HCI:EVENT:%02x\n", packet[0]);
    hci_dump_packet( HCI_EVENT_PACKET, 1, packet, size);
    
    switch (packet[0]) {
                        
        case HCI_EVENT_COMMAND_COMPLETE:
            // get num cmd packets
            // log_info("HCI_EVENT_COMMAND_COMPLETE cmds old %u - new %u\n", hci_stack->num_cmd_packets, packet[2]);
            hci_stack->num_cmd_packets = packet[2];
            
            if (COMMAND_COMPLETE_EVENT(packet, hci_read_buffer_size)){
                // from offset 5
                // status 
                // "The HC_ACL_Data_Packet_Length return parameter will be used to determine the size of the L2CAP segments contained in ACL Data Packets"
                hci_stack->acl_data_packet_length = READ_BT_16(packet, 6);
                // ignore: SCO data packet len (8)
                hci_stack->total_num_acl_packets  = packet[9];
                // ignore: total num SCO packets
                if (hci_stack->state == HCI_STATE_INITIALIZING){
                    // determine usable ACL payload size
                    if (HCI_ACL_PAYLOAD_SIZE < hci_stack->acl_data_packet_length){
                        hci_stack->acl_data_packet_length = HCI_ACL_PAYLOAD_SIZE;
                    }
                    printf("hci_read_buffer_size: used size %u, count %u\n",
                             hci_stack->acl_data_packet_length, hci_stack->total_num_acl_packets); 
                }
            }
#ifdef HAVE_BLE
            if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_buffer_size)){
                hci_stack->le_data_packet_length = READ_BT_16(packet, 6);
                hci_stack->total_num_le_packets  = packet[8];
                printf("hci_le_read_buffer_size: size %u, count %u\n", hci_stack->le_data_packet_length, hci_stack->total_num_le_packets);
            }            
#endif
            // Dump local address
            if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)) {
                bt_flip_addr(hci_stack->local_bd_addr, &packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE + 1]);
                log_info("Local Address, Status: 0x%02x: Addr: %s\n",
                    packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE], bd_addr_to_str(hci_stack->local_bd_addr));
            }
            if (COMMAND_COMPLETE_EVENT(packet, hci_write_scan_enable)){
                hci_emit_discoverable_enabled(hci_stack->discoverable);
            }
            // Note: HCI init checks 
            if (COMMAND_COMPLETE_EVENT(packet, hci_read_local_supported_features)){
                memcpy(hci_stack->local_supported_features, &packet[OFFSET_OF_DATA_IN_COMMAND_COMPLETE+1], 8);
                log_info("Local Supported Features: 0x%02x%02x%02x%02x%02x%02x%02x%02x",
                    hci_stack->local_supported_features[0], hci_stack->local_supported_features[1],
                    hci_stack->local_supported_features[2], hci_stack->local_supported_features[3],
                    hci_stack->local_supported_features[4], hci_stack->local_supported_features[5],
                    hci_stack->local_supported_features[6], hci_stack->local_supported_features[7]);

                // determine usable ACL packet types based buffer size and supported features
                hci_stack->packet_types = hci_acl_packet_types_for_buffer_size_and_local_features(hci_stack->acl_data_packet_length, &hci_stack->local_supported_features[0]);
                log_info("packet types %04x", hci_stack->packet_types); 

                // Classic/LE
                log_info("BR/EDR support %u, LE support %u", hci_classic_supported(), hci_le_supported());
            }
            break;
            
        case HCI_EVENT_COMMAND_STATUS:
            // get num cmd packets
            // log_info("HCI_EVENT_COMMAND_STATUS cmds - old %u - new %u\n", hci_stack->num_cmd_packets, packet[3]);
            hci_stack->num_cmd_packets = packet[3];
            break;
            
        case HCI_EVENT_NUMBER_OF_COMPLETED_PACKETS:
            for (i=0; i<packet[2];i++){
                handle = READ_BT_16(packet, 3 + 2*i);
                uint16_t num_packets = READ_BT_16(packet, 3 + packet[2]*2 + 2*i);
                conn = hci_connection_for_handle(handle);
                if (!conn){
                    log_error("hci_number_completed_packet lists unused con handle %u\n", handle);
                    continue;
                }
                conn->num_acl_packets_sent -= num_packets;
                log_info("hci_number_completed_packet %u processed for handle %u, outstanding %u\n", num_packets, handle, conn->num_acl_packets_sent);
            }
            break;
            
        case HCI_EVENT_CONNECTION_REQUEST:
            bt_flip_addr(addr, &packet[2]);
            // TODO: eval COD 8-10
            link_type = packet[11];
            log_info("Connection_incoming: %s, type %u\n", bd_addr_to_str(addr), link_type);
            if (link_type == 0 || link_type == 1 || link_type == 2) { // ACL or SCO
                conn = connection_for_address(addr, link_type);
                if (!conn) {
                    conn = create_connection_for_addr(addr, link_type);
                }
                if (!conn) {
                    // CONNECTION REJECTED DUE TO LIMITED RESOURCES (0X0D)
                    hci_stack->decline_reason = 0x0d;
                    BD_ADDR_COPY(hci_stack->decline_addr, addr);
                    break;
                }
                conn->state = RECEIVED_CONNECTION_REQUEST;
                hci_run();
            } else {
                // SYNCHRONOUS CONNECTION LIMIT TO A DEVICE EXCEEDED (0X0A)
                hci_stack->decline_reason = 0x0a;
                BD_ADDR_COPY(hci_stack->decline_addr, addr);
            }
            break;
            
        case HCI_EVENT_SYNCHRONOUS_CONNECTION_COMPLETE:
            log_info("eSCO Connection air mode: %d\n", packet[18]);
            // fall through
        case HCI_EVENT_CONNECTION_COMPLETE:
            // Connection management
            bt_flip_addr(addr, &packet[5]);
            link_type = packet[11];
            log_info("Connection_complete (status=%u) %s\n", packet[2], bd_addr_to_str(addr));
            conn = connection_for_address(addr, link_type);
            if (conn) {
                if (!packet[2]){
                    conn->state = OPEN;
                    conn->con_handle = READ_BT_16(packet, 3);
                    conn->bonding_flags |= BONDING_REQUEST_REMOTE_FEATURES;

                    if (conn->type == 1)
                    {
                        // restart timer
                        run_loop_set_timer(&conn->timeout, HCI_CONNECTION_TIMEOUT_MS);
                        run_loop_add_timer(&conn->timeout);
                    }

                    log_info("New connection: handle %u, %s\n", conn->con_handle, bd_addr_to_str(conn->address));
                    hci_emit_connection_complete(conn, 0);
                    hci_emit_nr_connections_changed();
                } else {
                    // notify client if dedicated bonding
                    if (conn->bonding_flags & BONDING_DEDICATED){
                        hci_emit_dedicated_bonding_result(conn, packet[2]);                        
                    }

                    // connection failed, remove entry
                    linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
                    btstack_memory_hci_connection_free( conn );
                    
                    // if authentication error, also delete link key
                    if (packet[2] == 0x05) {
                        hci_drop_link_key_for_bd_addr(&addr);
                    }
                }
            }
            break;

        case HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE:
            handle = READ_BT_16(packet, 3);
            conn = hci_connection_for_handle(handle);
            if (!conn) break;
            if (!packet[2]){
                uint8_t * features = &packet[5];
                if (features[6] & (1 << 3)){
                    conn->bonding_flags |= BONDING_REMOTE_SUPPORTS_SSP;
                }
            }
            conn->bonding_flags |= BONDING_RECEIVED_REMOTE_FEATURES;
            log_info("HCI_EVENT_READ_REMOTE_SUPPORTED_FEATURES_COMPLETE, bonding flags %x", conn->bonding_flags);
            if (conn->bonding_flags & BONDING_DEDICATED){
                conn->bonding_flags |= BONDING_SEND_AUTHENTICATE_REQUEST;
            }
            break;

        case HCI_EVENT_LINK_KEY_REQUEST:
            log_info("HCI_EVENT_LINK_KEY_REQUEST\n");
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], RECV_LINK_KEY_REQUEST);
            // non-bondable mode: link key negative reply will be sent by HANDLE_LINK_KEY_REQUEST
            if (hci_stack->bondable && !hci_stack->remote_device_db) break;
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], HANDLE_LINK_KEY_REQUEST);
            hci_run();
            // request handled by hci_run() as HANDLE_LINK_KEY_REQUEST gets set
            return;
            
        case HCI_EVENT_LINK_KEY_NOTIFICATION: {
            bt_flip_addr(addr, &packet[2]);
            conn = connection_for_address(addr, 1);
            if (!conn) break;
            conn->authentication_flags |= RECV_LINK_KEY_NOTIFICATION;
            link_key_type_t link_key_type = packet[24];
            // Change Connection Encryption keeps link key type
            if (link_key_type != CHANGED_COMBINATION_KEY){
                conn->link_key_type = link_key_type;
            }
            if (!hci_stack->remote_device_db) break;
            hci_stack->remote_device_db->put_link_key(&addr, (link_key_t *) &packet[8], conn->link_key_type);
            // still forward event to allow dismiss of pairing dialog
            break;
        }

        case HCI_EVENT_PIN_CODE_REQUEST:
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], LEGACY_PAIRING_ACTIVE);
            // non-bondable mode: pin code negative reply will be sent
            if (!hci_stack->bondable){
                hci_add_connection_flags_for_flipped_bd_addr(&packet[2], DENY_PIN_CODE_REQUEST);
                hci_run();
                return;
            }
            // PIN CODE REQUEST means the link key request didn't succee -> delete stored link key
            if (!hci_stack->remote_device_db) break;
            bt_flip_addr(addr, &packet[2]);
            hci_stack->remote_device_db->delete_link_key(&addr);
            break;
            
        case HCI_EVENT_IO_CAPABILITY_REQUEST:
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], RECV_IO_CAPABILITIES_REQUEST);
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], SEND_IO_CAPABILITIES_REPLY);
            break;
        
        case HCI_EVENT_USER_CONFIRMATION_REQUEST:
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], SSP_PAIRING_ACTIVE);
            if (!hci_stack->ssp_auto_accept) break;
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], SEND_USER_CONFIRM_REPLY);
            break;

        case HCI_EVENT_USER_PASSKEY_REQUEST:
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], SSP_PAIRING_ACTIVE);
            if (!hci_stack->ssp_auto_accept) break;
            hci_add_connection_flags_for_flipped_bd_addr(&packet[2], SEND_USER_PASSKEY_REPLY);
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            handle = READ_BT_16(packet, 3);
            conn = hci_connection_for_handle(handle);
            if (!conn) break;
            if (packet[2] == 0) {
                if (packet[5]){
                    conn->authentication_flags |= CONNECTION_ENCRYPTED;
                } else {
                    conn->authentication_flags &= ~CONNECTION_ENCRYPTED;
                }
            }
            hci_emit_security_level(handle, gap_security_level_for_connection(conn));
            break;

        case HCI_EVENT_AUTHENTICATION_COMPLETE_EVENT:
            handle = READ_BT_16(packet, 3);
            conn = hci_connection_for_handle(handle);
            if (!conn) break;

            // dedicated bonding: send result and disconnect
            if (conn->bonding_flags & BONDING_DEDICATED){
                conn->bonding_flags &= ~BONDING_DEDICATED;
                hci_emit_dedicated_bonding_result( conn, packet[2]);           
                conn->bonding_flags |= BONDING_DISCONNECT_DEDICATED_DONE;
                break;
            }

            if (packet[2] == 0 && gap_security_level_for_link_key_type(conn->link_key_type) >= conn->requested_security_level){
                // link key sufficient for requested security
                conn->bonding_flags |= BONDING_SEND_ENCRYPTION_REQUEST;
                break;
            }
            // not enough
            hci_emit_security_level(handle, gap_security_level_for_connection(conn));
            break;

#ifndef EMBEDDED
        case HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE:
            if (!hci_stack->remote_device_db) break;
            if (packet[2]) break; // status not ok
            bt_flip_addr(addr, &packet[3]);
            // fix for invalid remote names - terminate on 0xff
            for (i=0; i<248;i++){
                if (packet[9+i] == 0xff){
                    packet[9+i] = 0;
                    break;
                }
            }
            memset(&device_name, 0, sizeof(device_name_t));
            strncpy((char*) device_name, (char*) &packet[9], 248);
            hci_stack->remote_device_db->put_name(&addr, &device_name);
            break;
            
        case HCI_EVENT_INQUIRY_RESULT:
        case HCI_EVENT_INQUIRY_RESULT_WITH_RSSI:
            if (!hci_stack->remote_device_db) break;
            // first send inq result packet
            hci_stack->packet_handler(HCI_EVENT_PACKET, packet, size);
            // then send cached remote names
            for (i=0; i<packet[2];i++){
                bt_flip_addr(addr, &packet[3+i*6]);
                if (hci_stack->remote_device_db->get_name(&addr, &device_name)){
                    hci_emit_remote_name_cached(&addr, &device_name);
                }
            }
            return;
#endif
            
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            if (!packet[2]){
                handle = READ_BT_16(packet, 3);
                hci_connection_t * conn = hci_connection_for_handle(handle);
                if (conn) {
                    hci_shutdown_connection(conn);
                }
            }
            break;
            
        case HCI_EVENT_HARDWARE_ERROR:
            if(hci_stack->control && hci_stack->control->hw_error){
                (*hci_stack->control->hw_error)();
            }
            break;

        case HCI_EVENT_DATA_BUFFER_OVERFLOW:
            log_error("Data overrun\n");
            break;

        case HCI_EVENT_MODE_CHANGE_EVENT:
            {
                // data: event (8), len (8), status (8), handle (16), current_mode (8), interval (16)
                handle = READ_BT_16(packet, 3);
                hci_connection_t * conn = hci_connection_for_handle(handle);
                if (!packet[2])
                {
                    if (conn != 0)
                    {
                        uint8_t current_mode = packet[5];
                        uint16_t interval = READ_BT_16(packet,6);
                        log_debug("current_mode=%d interval=%d\n",current_mode,interval);
                        conn->mode = current_mode;
                    }
                }
                else
                {
                  log_debug("Change mode event failed with 0x%x\n", packet[2]);
                }
                break;
            }
#ifdef HAVE_BLE
        case HCI_EVENT_LE_META:
            switch (packet[2]) {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    // Connection management
                    bt_flip_addr(addr, &packet[8]);
                    log_info("LE Connection_complete (status=%u) %s\n", packet[3], bd_addr_to_str(addr));
                    // LE connections are auto-accepted, so just create a connection if there isn't one already
                    // TODO: if BLE only support ACL? Need confirm
                    conn = connection_for_address(addr, 1);
                    if (packet[3]){
                        if (conn){
                            // outgoing connection failed, remove entry
                            linked_list_remove(&hci_stack->connections, (linked_item_t *) conn);
                            btstack_memory_hci_connection_free( conn );
                            
                        }
                        // if authentication error, also delete link key
                        if (packet[3] == 0x05) {
                            hci_drop_link_key_for_bd_addr(&addr);
                        }
                        break;
                    }
                    if (!conn){
                        conn = create_connection_for_addr(addr, 1);
                    }
                    if (!conn){
                        // no memory
                        break;
                    }
                    
                    conn->state = OPEN;
                    conn->con_handle = READ_BT_16(packet, 4);
                    
                    // TODO: store - role, peer address type, conn_interval, conn_latency, supervision timeout, master clock

                    // restart timer
                    // run_loop_set_timer(&conn->timeout, HCI_CONNECTION_TIMEOUT_MS);
                    // run_loop_add_timer(&conn->timeout);
                    
                    log_info("New connection: handle %u, %s\n", conn->con_handle, bd_addr_to_str(conn->address));
                    
                    hci_emit_nr_connections_changed();
                    break;

            // printf("LE buffer size: %u, count %u\n", READ_BT_16(packet,6), packet[8]);
                    
                default:
                    break;
            }
            break;
#endif            
            
        default:
            break;
    }

    // handle BT initialization
    if (hci_stack->state == HCI_STATE_INITIALIZING){
        if (hci_stack->substate % 2){
            // odd: waiting for event
            if (packet[0] == HCI_EVENT_COMMAND_COMPLETE || packet[0] == HCI_EVENT_COMMAND_STATUS){
                // wait for explicit COMMAND COMPLETE on RESET
                if (hci_stack->substate > 1 || COMMAND_COMPLETE_EVENT(packet, hci_reset)) {
                    hci_stack->substate++;
                }
            }
        }
    }
    
    // help with BT sleep
    if (hci_stack->state == HCI_STATE_FALLING_ASLEEP
        && hci_stack->substate == 1
        && COMMAND_COMPLETE_EVENT(packet, hci_write_scan_enable)){
        hci_stack->substate++;
    }
    
    hci_stack->packet_handler(HCI_EVENT_PACKET, packet, size);
	
	// execute main loop
	hci_run();
}

static void packet_handler(uint8_t packet_type, uint8_t *packet, uint16_t size){
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            event_handler(packet, size);
            break;
        case HCI_ACL_DATA_PACKET:
            acl_handler(packet, size);
            break;
        default:
            break;
    }
}

/** Register HCI packet handlers */
void hci_register_packet_handler(void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size)){
    hci_stack->packet_handler = handler;
}

void hci_init(hci_transport_t *transport, void *config, bt_control_t *control, remote_device_db_t const* remote_device_db){
    
#ifdef HAVE_MALLOC
    if (!hci_stack) {
        hci_stack = (hci_stack_t*) malloc(sizeof(hci_stack_t));
    }
#else
    hci_stack = &hci_stack_static;
#endif
    memset(hci_stack, 0, sizeof(hci_stack_t));

    // reference to use transport layer implementation
    hci_stack->hci_transport = transport;
    
    // references to used control implementation
    hci_stack->control = control;
    
    // reference to used config
    hci_stack->config = config;
    
    // no connections yet
    hci_stack->connections = NULL;
    hci_stack->discoverable = 0;
    hci_stack->connectable = 0;
    hci_stack->bondable = 1;

    // no pending cmds
    hci_stack->decline_reason = 0;
    hci_stack->new_scan_enable_value = 0xff;
    
    // higher level handler
    hci_stack->packet_handler = dummy_handler;

    // store and open remote device db
    hci_stack->remote_device_db = remote_device_db;
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->open();
    }
    
    // max acl payload size defined in config.h
    hci_stack->acl_data_packet_length = HCI_ACL_PAYLOAD_SIZE;
    
    // register packet handlers with transport
    transport->register_packet_handler(&packet_handler);

    hci_stack->state = HCI_STATE_OFF;

    // class of device
    hci_stack->class_of_device = 0x007a020c; // Smartphone 

    // Secure Simple Pairing default: enable, no I/O capabilities, general bonding, mitm not required, auto accept 
    hci_stack->ssp_enable = 1;
    hci_stack->ssp_io_capability = SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT;
    hci_stack->ssp_authentication_requirement = SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_GENERAL_BONDING;
    hci_stack->ssp_auto_accept = 1;

    // LE
    hci_stack->adv_addr_type = 0;
    memset(hci_stack->adv_address, 0, 6);
}

void hci_close(){
    // close remote device db
    if (hci_stack->remote_device_db) {
        hci_stack->remote_device_db->close();
    }
    while (hci_stack->connections) {
        hci_shutdown_connection((hci_connection_t *) hci_stack->connections);
    }
    hci_power_control(HCI_POWER_OFF);
    
#ifdef HAVE_MALLOC
    free(hci_stack);
#endif
    hci_stack = NULL;
}

void hci_set_class_of_device(uint32_t class_of_device){
    hci_stack->class_of_device = class_of_device;
}

void hci_disable_l2cap_timeout_check(){
    disable_l2cap_timeouts = 1;
}
// State-Module-Driver overview
// state                    module  low-level 
// HCI_STATE_OFF             off      close
// HCI_STATE_INITIALIZING,   on       open
// HCI_STATE_WORKING,        on       open
// HCI_STATE_HALTING,        on       open
// HCI_STATE_SLEEPING,    off/sleep   close
// HCI_STATE_FALLING_ASLEEP  on       open

static int hci_power_control_on(void){
    
    // power on
    int err = 0;
    if (hci_stack->control && hci_stack->control->on){
        err = (*hci_stack->control->on)(hci_stack->config);
    }
    if (err){
        log_error( "POWER_ON failed\n");
        hci_emit_hci_open_failed();
        return err;
    }
    
    // open low-level device
    err = hci_stack->hci_transport->open(hci_stack->config);
    if (err){
        log_error( "HCI_INIT failed, turning Bluetooth off again\n");
        if (hci_stack->control && hci_stack->control->off){
            (*hci_stack->control->off)(hci_stack->config);
        }
        hci_emit_hci_open_failed();
        return err;
    }
    return 0;
}

static void hci_power_control_off(void){
    
    log_info("hci_power_control_off\n");

    // close low-level device
    hci_stack->hci_transport->close(hci_stack->config);

    log_info("hci_power_control_off - hci_transport closed\n");
    
    // power off
    if (hci_stack->control && hci_stack->control->off){
        (*hci_stack->control->off)(hci_stack->config);
    }
    
    log_info("hci_power_control_off - control closed\n");

    hci_stack->state = HCI_STATE_OFF;
}

static void hci_power_control_sleep(void){
    
    log_info("hci_power_control_sleep\n");
    
#if 0
    // don't close serial port during sleep
    
    // close low-level device
    hci_stack->hci_transport->close(hci_stack->config);
#endif
    
    // sleep mode
    if (hci_stack->control && hci_stack->control->sleep){
        (*hci_stack->control->sleep)(hci_stack->config);
    }
    
    hci_stack->state = HCI_STATE_SLEEPING;
}

static int hci_power_control_wake(void){
    
    log_info("hci_power_control_wake\n");

    // wake on
    if (hci_stack->control && hci_stack->control->wake){
        (*hci_stack->control->wake)(hci_stack->config);
    }
    
#if 0
    // open low-level device
    int err = hci_stack->hci_transport->open(hci_stack->config);
    if (err){
        log_error( "HCI_INIT failed, turning Bluetooth off again\n");
        if (hci_stack->control && hci_stack->control->off){
            (*hci_stack->control->off)(hci_stack->config);
        }
        hci_emit_hci_open_failed();
        return err;
    }
#endif
    
    return 0;
}


int hci_power_control(HCI_POWER_MODE power_mode){
    
    log_info("hci_power_control: %u, current mode %u\n", power_mode, hci_stack->state);
    
    int err = 0;
    switch (hci_stack->state){
            
        case HCI_STATE_OFF:
            switch (power_mode){
                case HCI_POWER_ON:
                    err = hci_power_control_on();
                    if (err) return err;
                    // set up state machine
                    hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                    hci_stack->state = HCI_STATE_INITIALIZING;
                    hci_stack->substate = 0;
                    break;
                case HCI_POWER_OFF:
                    // do nothing
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing (with SLEEP == OFF)
                    break;
            }
            break;
            
        case HCI_STATE_INITIALIZING:
            switch (power_mode){
                case HCI_POWER_ON:
                    // do nothing
                    break;
                case HCI_POWER_OFF:
                    // no connections yet, just turn it off
                    hci_power_control_off();
                    break;  
                case HCI_POWER_SLEEP:
                    // no connections yet, just turn it off
                    hci_power_control_sleep();
                    break;
            }
            break;
            
        case HCI_STATE_WORKING:
            switch (power_mode){
                case HCI_POWER_ON:
                    // do nothing
                    break;
                case HCI_POWER_OFF:
                    // see hci_run
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // see hci_run
                    hci_stack->state = HCI_STATE_FALLING_ASLEEP;
                    hci_stack->substate = 0;
                    break;
            }
            break;
            
        case HCI_STATE_HALTING:
            switch (power_mode){
                case HCI_POWER_ON:
                    // set up state machine
                    hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                    hci_stack->state = HCI_STATE_INITIALIZING;
                    hci_stack->substate = 0;
                    break;
                case HCI_POWER_OFF:
                    // do nothing
                    break;  
                case HCI_POWER_SLEEP:
                    // see hci_run
                    hci_stack->state = HCI_STATE_FALLING_ASLEEP;
                    hci_stack->substate = 0;
                    break;
            }
            break;
            
        case HCI_STATE_FALLING_ASLEEP:
            switch (power_mode){
                case HCI_POWER_ON:

#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // nothing to do, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                        hci_stack->state = HCI_STATE_INITIALIZING;
                        hci_stack->substate = HCI_INTIALIZING_SUBSTATE_AFTER_SLEEP;
                        break;
                    }
#endif
                    // set up state machine
                    hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                    hci_stack->state = HCI_STATE_INITIALIZING;
                    hci_stack->substate = 0;
                    break;
                case HCI_POWER_OFF:
                    // see hci_run
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing
                    break;
            }
            break;
            
        case HCI_STATE_SLEEPING:
            switch (power_mode){
                case HCI_POWER_ON:
                    
#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // nothing to do, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                        hci_stack->state = HCI_STATE_INITIALIZING;
                        hci_stack->substate = HCI_INTIALIZING_SUBSTATE_AFTER_SLEEP;
                        hci_update_scan_enable();
                        break;
                    }
#endif
                    err = hci_power_control_wake();
                    if (err) return err;
                    // set up state machine
                    hci_stack->num_cmd_packets = 1; // assume that one cmd can be sent
                    hci_stack->state = HCI_STATE_INITIALIZING;
                    hci_stack->substate = 0;
                    break;
                case HCI_POWER_OFF:
                    hci_stack->state = HCI_STATE_HALTING;
                    break;  
                case HCI_POWER_SLEEP:
                    // do nothing
                    break;
            }
            break;
    }

    // create internal event
	hci_emit_state();
    
	// trigger next/first action
	hci_run();
	
    return 0;
}

static void hci_update_scan_enable(void){
    // 2 = page scan, 1 = inq scan
    hci_stack->new_scan_enable_value  = hci_stack->connectable << 1 | hci_stack->discoverable;
    hci_run();
}

void hci_discoverable_control(uint8_t enable){
    if (enable) enable = 1; // normalize argument
    
    if (hci_stack->discoverable == enable){
        hci_emit_discoverable_enabled(hci_stack->discoverable);
        return;
    }

    hci_stack->discoverable = enable;
    hci_update_scan_enable();
}

void hci_connectable_control(uint8_t enable){
    if (enable) enable = 1; // normalize argument
    
    // don't emit event
    if (hci_stack->connectable == enable) return;

    hci_stack->connectable = enable;
    hci_update_scan_enable();
}

bd_addr_t * hci_local_bd_addr(void){
    return &hci_stack->local_bd_addr;
}

void hci_run(){
        
    hci_connection_t * connection;
    linked_item_t * it;
    
    if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;

    // global/non-connection oriented commands
    
    // decline incoming connections
    if (hci_stack->decline_reason){
        uint8_t reason = hci_stack->decline_reason;
        hci_stack->decline_reason = 0;
        hci_send_cmd(&hci_reject_connection_request, hci_stack->decline_addr, reason);
        return;
    }

    // send scan enable
    if (hci_stack->state == HCI_STATE_WORKING && hci_stack->new_scan_enable_value != 0xff && hci_classic_supported()){
        hci_send_cmd(&hci_write_scan_enable, hci_stack->new_scan_enable_value);
        hci_stack->new_scan_enable_value = 0xff;
        return;
    }
    
    // send pending HCI commands
    for (it = (linked_item_t *) hci_stack->connections; it ; it = it->next){

        connection = (hci_connection_t *) it;
        
        // check if sniff mode is correct
        if (connection->mode & EXIT_SNIFF)
        {
            connection->mode &= ~EXIT_SNIFF;
            if ((connection->mode & MODE_VALUES) == SNIFF)
            {
                log_debug("sending hci_exit_sniff_mode\n");
                hci_send_cmd(&hci_exit_sniff_mode,connection->con_handle);
                return;
            }
        }
        else if (connection->mode & ENTER_SNIFF)
        {
            connection->mode &= ~ENTER_SNIFF;
            if ((connection->mode & MODE_VALUES) == ACTIVE)
            {
                log_debug("sending hci_sniff_mode\n");
                hci_send_cmd(&hci_sniff_mode,connection->con_handle,sniff_max_interval,sniff_min_interval,sniff_attempt,sniff_timeout);
            }
        }

        if (connection->state == SEND_CREATE_CONNECTION){
            log_info("sending hci_create_connection\n");
            hci_send_cmd(&hci_create_connection, connection->address, hci_usable_acl_packet_types(), 0, 0, 0, 1); 
            return;
        }

        if (connection->state == RECEIVED_CONNECTION_REQUEST){
            if (connection->type == 1)
            {
                log_info("sending hci_accept_connection_request\n");
                hci_send_cmd(&hci_accept_connection_request, connection->address, 1);
            }
            else if (connection->type == 2 || connection->type == 0) // SCO or eSCO
            {
                log_info("hci_accept_synchronous_connection\n");
                hci_send_cmd(&hci_accept_synchronous_connection, connection->address, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF,
                    0x20, 0x01, 0x003F);
            }
            connection->state = ACCEPTED_CONNECTION_REQUEST;
            return;
        }
        
        if (connection->authentication_flags & HANDLE_LINK_KEY_REQUEST){
            log_info("responding to link key request\n");
            connectionClearAuthenticationFlags(connection, HANDLE_LINK_KEY_REQUEST);
            link_key_t link_key;
            link_key_type_t link_key_type;
            if ( hci_stack->remote_device_db
              && hci_stack->remote_device_db->get_link_key( &connection->address, &link_key, &link_key_type)
              && gap_security_level_for_link_key_type(link_key_type) >= connection->requested_security_level){
               connection->link_key_type = link_key_type;
               hci_send_cmd(&hci_link_key_request_reply, connection->address, &link_key);
            } else {
               hci_send_cmd(&hci_link_key_request_negative_reply, connection->address);
            }
            return;
        }

        if (connection->authentication_flags & DENY_PIN_CODE_REQUEST){
            log_info("denying to pin request\n");
            connectionClearAuthenticationFlags(connection, DENY_PIN_CODE_REQUEST);
            hci_send_cmd(&hci_pin_code_request_negative_reply, connection->address);
            return;
        }

        if (connection->authentication_flags & SEND_IO_CAPABILITIES_REPLY){
            connectionClearAuthenticationFlags(connection, SEND_IO_CAPABILITIES_REPLY);
            log_info("IO Capability Request received, stack bondable %u, io cap %u", hci_stack->bondable, hci_stack->ssp_io_capability);
            if (hci_stack->bondable && (hci_stack->ssp_io_capability != SSP_IO_CAPABILITY_UNKNOWN)){
                // tweak authentication requirements
                uint8_t authreq = hci_stack->ssp_authentication_requirement;
                if (connection->bonding_flags & BONDING_DEDICATED){
                    authreq = SSP_IO_AUTHREQ_MITM_PROTECTION_NOT_REQUIRED_DEDICATED_BONDING;
                }
                if (gap_mitm_protection_required_for_security_level(connection->requested_security_level)){
                    authreq |= 1;
                } 
                hci_send_cmd(&hci_io_capability_request_reply, &connection->address, hci_stack->ssp_io_capability, NULL, authreq);
            } else {
                hci_send_cmd(&hci_io_capability_request_negative_reply, &connection->address, ERROR_CODE_PAIRING_NOT_ALLOWED);
            }
            return;
        }
        
        if (connection->authentication_flags & SEND_USER_CONFIRM_REPLY){
            connectionClearAuthenticationFlags(connection, SEND_USER_CONFIRM_REPLY);
            hci_send_cmd(&hci_user_confirmation_request_reply, &connection->address);
            return;
        }

        if (connection->authentication_flags & SEND_USER_PASSKEY_REPLY){
            connectionClearAuthenticationFlags(connection, SEND_USER_PASSKEY_REPLY);
            hci_send_cmd(&hci_user_passkey_request_reply, &connection->address, 000000);
            return;
        }

        if (connection->bonding_flags & BONDING_REQUEST_REMOTE_FEATURES){
            connection->bonding_flags &= ~BONDING_REQUEST_REMOTE_FEATURES;
            hci_send_cmd(&hci_read_remote_supported_features_command, connection->con_handle);
            return;
        }

        if (connection->bonding_flags & BONDING_DISCONNECT_SECURITY_BLOCK){
            connection->bonding_flags &= ~BONDING_DISCONNECT_SECURITY_BLOCK;
            hci_send_cmd(&hci_disconnect, connection->con_handle, 0x0005);  // authentication failure
            return;
        }
        if (connection->bonding_flags & BONDING_DISCONNECT_DEDICATED_DONE){
            connection->bonding_flags &= ~BONDING_DISCONNECT_DEDICATED_DONE;
            hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // authentication done
            return;
        }
        if (connection->bonding_flags & BONDING_SEND_AUTHENTICATE_REQUEST){
            connection->bonding_flags &= ~BONDING_SEND_AUTHENTICATE_REQUEST;
            hci_send_cmd(&hci_authentication_requested, connection->con_handle);
            return;
        }
        if (connection->bonding_flags & BONDING_SEND_ENCRYPTION_REQUEST){
            connection->bonding_flags &= ~BONDING_SEND_ENCRYPTION_REQUEST;
            hci_send_cmd(&hci_set_connection_encryption, connection->con_handle, 1);
            return;
        }
    }

    switch (hci_stack->state){
        case HCI_STATE_INITIALIZING:
            // log_info("hci_init: substate %u\n", hci_stack->substate);
            if (hci_stack->substate % 2) {
                // odd: waiting for command completion
                return;
            }
            switch (hci_stack->substate >> 1){
                case 0: // RESET
                    hci_send_cmd(&hci_reset);

                    if (hci_stack->config == 0 || ((hci_uart_config_t *)hci_stack->config)->baudrate_main == 0){
                        // skip baud change
                        hci_stack->substate = 4; // >> 1 = 2
                    }
                    break;
                case 1: // SEND BAUD CHANGE
                    hci_stack->control->baudrate_cmd(hci_stack->config, ((hci_uart_config_t *)hci_stack->config)->baudrate_main, hci_stack->hci_packet_buffer);
                    hci_send_cmd_packet(hci_stack->hci_packet_buffer, 3 + hci_stack->hci_packet_buffer[2]);
                    break;
                case 2: // LOCAL BAUD CHANGE
                    hci_stack->hci_transport->set_baudrate(((hci_uart_config_t *)hci_stack->config)->baudrate_main);
                    hci_stack->substate += 2;
                    // break missing here for fall through
                    
                case 3:
                    // Custom initialization
                    if (hci_stack->control && hci_stack->control->next_cmd){
                        int valid_cmd = (*hci_stack->control->next_cmd)(hci_stack->config, hci_stack->hci_packet_buffer);
                        if (valid_cmd){
                            int size = 3 + hci_stack->hci_packet_buffer[2];
                            hci_stack->hci_transport->send_packet(HCI_COMMAND_DATA_PACKET, hci_stack->hci_packet_buffer, size);
                            hci_stack->substate = 4; // more init commands
                            break;
                        }
                        log_info("hci_run: init script done\n\r");
                    }
                    // otherwise continue
					hci_send_cmd(&hci_read_bd_addr);
					break;
				case 4:
					hci_send_cmd(&hci_read_buffer_size);
					break;
                case 5:
                    hci_send_cmd(&hci_read_local_supported_features);
                    break;                
                case 6:
                    if (hci_le_supported()){
                        hci_send_cmd(&hci_set_event_mask,0xffffffff, 0x3FFFFFFF);
                    } else {
                        // Kensington Bluetoot 2.1 USB Dongle (CSR Chipset) returns an error for 0xffff... 
                        hci_send_cmd(&hci_set_event_mask,0xffffffff, 0x1FFFFFFF);
                    }

                    // skip Classic init commands for LE only chipsets
                    if (!hci_classic_supported()){
                        if (hci_le_supported()){
                            hci_stack->substate = 11 << 1;    // skip all classic command
                        } else {
                            log_error("Neither BR/EDR nor LE supported");
                            hci_stack->substate = 13 << 1;    // skip all
                        }
                    }
                    break;
                case 7:
                    if (hci_ssp_supported()){
                        hci_send_cmd(&hci_write_simple_pairing_mode, hci_stack->ssp_enable);
                        break;
                    }
                    hci_stack->substate += 2;
                    // break missing here for fall through

                case 8:
                    // ca. 15 sec
                    hci_send_cmd(&hci_write_page_timeout, 0x6000);
                    break;
                case 9:
                    hci_send_cmd(&hci_write_class_of_device, hci_stack->class_of_device);
                    break;
                case 10:
                    if (hci_stack->local_name){
                        hci_send_cmd(&hci_write_local_name, hci_stack->local_name);
                    } else {
                        char hostname[30];
#ifdef EMBEDDED
                        const char* addr = system_getserial();
                        sprintf(hostname, "Meteor %02X%02X", addr[4], addr[5]);
                        printf("---> Name %s\n", hostname);
#else
                        // hostname for POSIX systems
                        gethostname(hostname, 30);
                        hostname[29] = '\0';
#endif                        
                        hci_send_cmd(&hci_write_local_name, hostname);
                    }
                    break;
                case 11:
					hci_send_cmd(&hci_write_scan_enable, (hci_stack->connectable << 1) | hci_stack->discoverable); // page scan
                    if (!hci_le_supported()){
                        // SKIP LE init for Classic only configuration
                        hci_stack->substate = 13 << 1;
                    }
					break;

#ifdef HAVE_BLE
                // LE INIT
                case 12:
                    hci_send_cmd(&hci_le_read_buffer_size);
                    break;
                case 13:
                    // LE Supported Host = 1, Simultaneous Host = 0
                    hci_send_cmd(&hci_write_le_host_supported, 1, 0);
                    break;
#endif

                // DONE
                case 14:
                    // done.
                    hci_stack->state = HCI_STATE_WORKING;
                    hci_emit_state();
                    break;
                default:
                    break;
            }
            hci_stack->substate++;
            break;
            
        case HCI_STATE_HALTING:

            log_info("HCI_STATE_HALTING\n");
            // close all open connections
            connection =  (hci_connection_t *) hci_stack->connections;
            if (connection){
                
                // send disconnect
                if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;
                
                log_info("HCI_STATE_HALTING, connection %p, handle %u\n", connection, (uint16_t)connection->con_handle);
                hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // remote closed connection

                // send disconnected event right away - causes higher layer connections to get closed, too.
                hci_shutdown_connection(connection);
                return;
            }
            log_info("HCI_STATE_HALTING, calling off\n");
            
            // switch mode
            hci_power_control_off();
            
            log_info("HCI_STATE_HALTING, emitting state\n");
            hci_emit_state();
            log_info("HCI_STATE_HALTING, done\n");
            break;
            
        case HCI_STATE_FALLING_ASLEEP:
            switch(hci_stack->substate) {
                case 0:
                    log_info("HCI_STATE_FALLING_ASLEEP\n");
                    // close all open connections
                    connection =  (hci_connection_t *) hci_stack->connections;

#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // don't close connections, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        connection = NULL;
                    }
#endif
                    if (connection){
                        
                        // send disconnect
                        if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;

                        log_info("HCI_STATE_FALLING_ASLEEP, connection %p, handle %u\n", connection, (uint16_t)connection->con_handle);
                        hci_send_cmd(&hci_disconnect, connection->con_handle, 0x13);  // remote closed connection
                        
                        // send disconnected event right away - causes higher layer connections to get closed, too.
                        hci_shutdown_connection(connection);
                        return;
                    }
                    
                    if (hci_classic_supported()){
                        // disable page and inquiry scan
                        if (!hci_can_send_packet_now(HCI_COMMAND_DATA_PACKET)) return;
                        
                        log_info("HCI_STATE_HALTING, disabling inq scans\n");
                        hci_send_cmd(&hci_write_scan_enable, hci_stack->connectable << 1); // drop inquiry scan but keep page scan
                        
                        // continue in next sub state
                        hci_stack->substate++;
                        break;
                    }
                    // fall through for ble-only chips

                case 2:
                    log_info("HCI_STATE_HALTING, calling sleep\n");
#if defined(USE_POWERMANAGEMENT) && defined(USE_BLUETOOL)
                    // don't actually go to sleep, if H4 supports power management
                    if (bt_control_iphone_power_management_enabled()){
                        // SLEEP MODE reached
                        hci_stack->state = HCI_STATE_SLEEPING; 
                        hci_emit_state();
                        break;
                    }
#endif
                    // switch mode
                    hci_power_control_sleep();  // changes hci_stack->state to SLEEP
                    hci_emit_state();
                    break;
                    
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}

void hci_set_sniff_timeout(hci_con_handle_t handle, uint16_t timeout)
{
    hci_connection_t *connection = hci_connection_for_handle( handle);
    if (connection) 
    {
        connection->sniff_timeout = timeout;
        if (timeout == -1)
        {
            connection->mode |= EXIT_SNIFF;
        }
        else
        {
          if (timeout < HCI_CONNECTION_TIMEOUT_MS)
          {
            run_loop_remove_timer(&connection->timeout);
            run_loop_set_timer(&connection->timeout, timeout);
            run_loop_add_timer(&connection->timeout);
          }
        }
    }

    hci_run();
}

void hci_exit_sniff(hci_con_handle_t handle)
{
    hci_connection_t *connection = hci_connection_for_handle( handle);
    if (connection) 
    {
        connection->mode |= EXIT_SNIFF;
    }

    hci_run();
}

int hci_send_cmd_packet(uint8_t *packet, int size){
    bd_addr_t addr;
    hci_connection_t * conn;
    uint8_t link_type;
    // house-keeping
    hci_dump_packet( HCI_COMMAND_DATA_PACKET, 0, packet, size);
    
    // create_connection?
    if (IS_COMMAND(packet, hci_create_connection)){
        bt_flip_addr(addr, &packet[3]);
        link_type = packet[9];
        log_info("Create_connection to %s\n", bd_addr_to_str(addr));

        conn = connection_for_address(addr, link_type);
        if (!conn){
            conn = create_connection_for_addr(addr, link_type);
            if (!conn){
                // notify client that alloc failed
                hci_emit_connection_complete(conn, BTSTACK_MEMORY_ALLOC_FAILED);
                return 0; // don't sent packet to controller
            }
            conn->state = SEND_CREATE_CONNECTION;
        }
        log_info("conn state %u", conn->state);
        switch (conn->state){
            // if connection active exists
            case OPEN:
                // and OPEN, emit connection complete command, don't send to controller
                hci_emit_connection_complete(conn, 0);
                return 0;
            case SEND_CREATE_CONNECTION:
                // connection created by hci, e.g. dedicated bonding
                break;
            default:
                // otherwise, just ignore as it is already in the open process
                return 0;
        }
        conn->state = SENT_CREATE_CONNECTION;
    }
    
    if (IS_COMMAND(packet, hci_link_key_request_reply)){
        hci_add_connection_flags_for_flipped_bd_addr(&packet[3], SENT_LINK_KEY_REPLY);
    }
    if (IS_COMMAND(packet, hci_link_key_request_negative_reply)){
        hci_add_connection_flags_for_flipped_bd_addr(&packet[3], SENT_LINK_KEY_NEGATIVE_REQUEST);
    }
    
    if (IS_COMMAND(packet, hci_delete_stored_link_key)){
        if (hci_stack->remote_device_db){
            bt_flip_addr(addr, &packet[3]);
            hci_stack->remote_device_db->delete_link_key(&addr);
        }
    }

    if (IS_COMMAND(packet, hci_pin_code_request_negative_reply)
    ||  IS_COMMAND(packet, hci_pin_code_request_reply)){
        bt_flip_addr(addr, &packet[3]);
        conn = connection_for_address(addr, 1);
        if (conn){
            connectionClearAuthenticationFlags(conn, LEGACY_PAIRING_ACTIVE);
        }
    }

    if (IS_COMMAND(packet, hci_user_confirmation_request_negative_reply)
    ||  IS_COMMAND(packet, hci_user_confirmation_request_reply)
    ||  IS_COMMAND(packet, hci_user_passkey_request_negative_reply)
    ||  IS_COMMAND(packet, hci_user_passkey_request_reply)) {
        bt_flip_addr(addr, &packet[3]);
        conn = connection_for_address(addr, 1);
        if (conn){
            connectionClearAuthenticationFlags(conn, SSP_PAIRING_ACTIVE);
        }
    }

#ifdef HAVE_BLE
    if (IS_COMMAND(packet, hci_le_set_advertising_parameters)){
        hci_stack->adv_addr_type = packet[8];
    }
    if (IS_COMMAND(packet, hci_le_set_random_address)){
        bt_flip_addr(hci_stack->adv_address, &packet[3]);
    }
#endif


    hci_stack->num_cmd_packets--;
    return hci_stack->hci_transport->send_packet(HCI_COMMAND_DATA_PACKET, packet, size);
}

// disconnect because of security block
void hci_disconnect_security_block(hci_con_handle_t con_handle){
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return;
    connection->bonding_flags |= BONDING_DISCONNECT_SECURITY_BLOCK;
}


// Configure Secure Simple Pairing

// enable will enable SSP during init
void hci_ssp_set_enable(int enable){
    hci_stack->ssp_enable = enable;
}

int hci_local_ssp_activated(){
    return hci_ssp_supported() && hci_stack->ssp_enable;
}

// if set, BTstack will respond to io capability request using authentication requirement
void hci_ssp_set_io_capability(int io_capability){
    hci_stack->ssp_io_capability = io_capability;
}
void hci_ssp_set_authentication_requirement(int authentication_requirement){
    hci_stack->ssp_authentication_requirement = authentication_requirement;
}

// if set, BTstack will confirm a numberic comparion and enter '000000' if requested
void hci_ssp_set_auto_accept(int auto_accept){
    hci_stack->ssp_auto_accept = auto_accept;
}

/**
 * pre: numcmds >= 0 - it's allowed to send a command to the controller
 */
int hci_send_cmd(const hci_cmd_t *cmd, ...){
    va_list argptr;
    va_start(argptr, cmd);
    uint16_t size = hci_create_cmd_internal(hci_stack->hci_packet_buffer, cmd, argptr);
    va_end(argptr);
    return hci_send_cmd_packet(hci_stack->hci_packet_buffer, size);
}

// Create various non-HCI events. 
// TODO: generalize, use table similar to hci_create_command

void hci_emit_state(){
    log_info("BTSTACK_EVENT_STATE %u", hci_stack->state);
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_STATE;
    event[1] = sizeof(event) - 2;
    event[2] = hci_stack->state;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_connection_complete(hci_connection_t *conn, uint8_t status){
    uint8_t event[13];
    event[0] = HCI_EVENT_CONNECTION_COMPLETE;
    event[1] = sizeof(event) - 2;
    event[2] = status;
    bt_store_16(event, 3, conn->con_handle);
    bt_flip_addr(&event[5], conn->address);
    event[11] = conn->type; // ACL connection
    event[12] = 0; // encryption disabled
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_disconnection_complete(uint16_t handle, uint8_t reason){
    uint8_t event[6];
    event[0] = HCI_EVENT_DISCONNECTION_COMPLETE;
    event[1] = sizeof(event) - 2;
    event[2] = 0; // status = OK
    bt_store_16(event, 3, handle);
    event[5] = reason;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_l2cap_check_timeout(hci_connection_t *conn){
    if (disable_l2cap_timeouts) return;
    log_info("L2CAP_EVENT_TIMEOUT_CHECK");
    uint8_t event[4];
    event[0] = L2CAP_EVENT_TIMEOUT_CHECK;
    event[1] = sizeof(event) - 2;
    bt_store_16(event, 2, conn->con_handle);
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_nr_connections_changed(){
    log_info("BTSTACK_EVENT_NR_CONNECTIONS_CHANGED %u", nr_hci_connections());
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_NR_CONNECTIONS_CHANGED;
    event[1] = sizeof(event) - 2;
    event[2] = nr_hci_connections();
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_hci_open_failed(){
    log_info("BTSTACK_EVENT_POWERON_FAILED");
    uint8_t event[2];
    event[0] = BTSTACK_EVENT_POWERON_FAILED;
    event[1] = sizeof(event) - 2;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

#ifndef EMBEDDED
void hci_emit_btstack_version() {
    log_info("BTSTACK_EVENT_VERSION %u.%u", BTSTACK_MAJOR, BTSTACK_MINOR);
    uint8_t event[6];
    event[0] = BTSTACK_EVENT_VERSION;
    event[1] = sizeof(event) - 2;
    event[2] = BTSTACK_MAJOR;
    event[3] = BTSTACK_MINOR;
    bt_store_16(event, 4, BTSTACK_REVISION);
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}
#endif

void hci_emit_system_bluetooth_enabled(uint8_t enabled){
    log_info("BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED %u", enabled);
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_SYSTEM_BLUETOOTH_ENABLED;
    event[1] = sizeof(event) - 2;
    event[2] = enabled;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_remote_name_cached(bd_addr_t *addr, device_name_t *name){
    uint8_t event[2+1+6+248+1]; // +1 for \0 in log_info
    event[0] = BTSTACK_EVENT_REMOTE_NAME_CACHED;
    event[1] = sizeof(event) - 2 - 1;
    event[2] = 0;   // just to be compatible with HCI_EVENT_REMOTE_NAME_REQUEST_COMPLETE
    bt_flip_addr(&event[3], *addr);
    memcpy(&event[9], name, 248);
    
    event[9+248] = 0;   // assert \0 for log_info
    log_info("BTSTACK_EVENT_REMOTE_NAME_CACHED %s = '%s'", bd_addr_to_str(*addr), &event[9]);

    hci_dump_packet(HCI_EVENT_PACKET, 0, event, sizeof(event)-1);
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event)-1);
}

void hci_emit_discoverable_enabled(uint8_t enabled){
    log_info("BTSTACK_EVENT_DISCOVERABLE_ENABLED %u", enabled);
    uint8_t event[3];
    event[0] = BTSTACK_EVENT_DISCOVERABLE_ENABLED;
    event[1] = sizeof(event) - 2;
    event[2] = enabled;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_security_level(hci_con_handle_t con_handle, gap_security_level_t level){
    log_info("hci_emit_security_level %u for handle %x", level, con_handle);
    uint8_t event[5];
    int pos = 0;
    event[pos++] = GAP_SECURITY_LEVEL;
    event[pos++] = sizeof(event) - 2;
    bt_store_16(event, 2, con_handle);
    pos += 2;
    event[pos++] = level;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

void hci_emit_dedicated_bonding_result(hci_connection_t * connection, uint8_t status){
    log_info("hci_emit_dedicated_bonding_result %u ", status);
    uint8_t event[9];
    int pos = 0;
    event[pos++] = GAP_DEDICATED_BONDING_COMPLETED;
    event[pos++] = sizeof(event) - 2;
    event[pos++] = status;
    bt_flip_addr( * (bd_addr_t *) &event[pos], connection->address);
    pos += 6;
    hci_dump_packet( HCI_EVENT_PACKET, 0, event, sizeof(event));
    hci_stack->packet_handler(HCI_EVENT_PACKET, event, sizeof(event));
}

// query if remote side supports SSP
int hci_remote_ssp_supported(hci_con_handle_t con_handle){
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return 0;
    return (connection->bonding_flags & BONDING_REMOTE_SUPPORTS_SSP) ? 1 : 0;
}

int hci_ssp_supported_on_both_sides(hci_con_handle_t handle){
    return hci_local_ssp_activated() && hci_remote_ssp_supported(handle);
}

// GAP API
/**
 * @bbrief enable/disable bonding. default is enabled
 * @praram enabled
 */
void gap_set_bondable_mode(int enable){
    hci_stack->bondable = enable ? 1 : 0;
}

/**
 * @brief map link keys to security levels
 */
gap_security_level_t gap_security_level_for_link_key_type(link_key_type_t link_key_type){
    switch (link_key_type){
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P256:
            return LEVEL_4;
        case COMBINATION_KEY:
        case AUTHENTICATED_COMBINATION_KEY_GENERATED_FROM_P192:
            return LEVEL_3;
        default:
            return LEVEL_2;
    }
}

static gap_security_level_t gap_security_level_for_connection(hci_connection_t * connection){
    if (!connection) return LEVEL_0;
    if ((connection->authentication_flags & CONNECTION_ENCRYPTED) == 0) return LEVEL_0;
    return gap_security_level_for_link_key_type(connection->link_key_type);
}    


int gap_mitm_protection_required_for_security_level(gap_security_level_t level){
    printf("gap_mitm_protection_required_for_security_level %u\n", level);
    return level > LEVEL_2;
}

/**
 * @brief get current security level
 */
gap_security_level_t gap_security_level(hci_con_handle_t con_handle){
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection) return LEVEL_0;
    return gap_security_level_for_connection(connection);
}

/**
 * @brief request connection to device to
 * @result GAP_AUTHENTICATION_RESULT
 */
void gap_request_security_level(hci_con_handle_t con_handle, gap_security_level_t requested_level){
    hci_connection_t * connection = hci_connection_for_handle(con_handle);
    if (!connection){
        hci_emit_security_level(con_handle, LEVEL_0);
        return;
    }
    gap_security_level_t current_level = gap_security_level(con_handle);
    log_info("gap_request_security_level %u, current level %u", requested_level, current_level);
    if (current_level >= requested_level){
        hci_emit_security_level(con_handle, current_level);
        return;
    }

    connection->requested_security_level = requested_level;

    // would enabling ecnryption suffice (>= LEVEL_2)?
    if (hci_stack->remote_device_db){
        link_key_type_t link_key_type;
        link_key_t      link_key;
        if (hci_stack->remote_device_db->get_link_key( &connection->address, &link_key, &link_key_type)){
            if (gap_security_level_for_link_key_type(link_key_type) >= requested_level){
                connection->bonding_flags |= BONDING_SEND_ENCRYPTION_REQUEST;
                return;
            }
        }
    }

    // try to authenticate connection
    connection->bonding_flags |= BONDING_SEND_AUTHENTICATE_REQUEST;
    hci_run();
}

/**
 * @brief start dedicated bonding with device. disconnect after bonding
 * @param device
 * @param request MITM protection
 * @result GAP_DEDICATED_BONDING_COMPLETE
 */
int gap_dedicated_bonding(bd_addr_t device, int mitm_protection_required){

    // create connection state machine
    hci_connection_t * connection = create_connection_for_addr(device, 1);

    if (!connection){
        return BTSTACK_MEMORY_ALLOC_FAILED;
    }

    // delete linkn key
    hci_drop_link_key_for_bd_addr( (bd_addr_t *) &device);

    // configure LEVEL_2/3, dedicated bonding
    connection->state = SEND_CREATE_CONNECTION;    
    connection->requested_security_level = mitm_protection_required ? LEVEL_3 : LEVEL_2;
    printf("gap_dedicated_bonding, mitm %u -> level %u\n", mitm_protection_required, connection->requested_security_level);
    connection->bonding_flags = BONDING_DEDICATED;

    // wait for GAP Security Result and send GAP Dedicated Bonding complete

    // handle: connnection failure (connection complete != ok)
    // handle: authentication failure
    // handle: disconnect on done

    hci_run();

    return 0;
}

void gap_set_local_name(const char * local_name){
    hci_stack->local_name = local_name;
}


uint8_t hci_le_data_packet_length()
{
    return hci_stack->le_data_packet_length - 1;
}