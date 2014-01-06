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

le_peripheral_t test_device;
le_service_t services[20];
int service_count = 0;
int service_index = 0;

// static bd_addr_t test_device_addr = {0x1c, 0xba, 0x8c, 0x20, 0xc7, 0xf6};
static bd_addr_t test_device_addr = {0x00, 0x1b, 0xdc, 0x05, 0xb5, 0xdc};

static void dump_ad_event(ad_event_t* e){
    printf("evt-type %u, addr-type %u, addr %s, rssi %u, length adv %u, data: ", e->event_type, 
            e->address_type, bd_addr_to_str(e->address), e->rssi, e->length); 
    hexdump2( e->data, e->length);                            
}

static void dump_peripheral_state(peripheral_state_t p_state){
    switch(p_state) {
        case P_W2_CONNECT: printf("P_W2_CONNECT"); break;
        case P_W4_CONNECTED: printf("P_W4_CONNECTED"); break;
        case P_W2_EXCHANGE_MTU: printf("P_W2_EXCHANGE_MTU"); break;
        case P_W4_EXCHANGE_MTU: printf("P_W4_EXCHANGE_MTU"); break;
        case P_CONNECTED: printf("P_CONNECTED"); break;
        case P_W2_CANCEL_CONNECT: printf("P_W2_CANCEL_CONNECT"); break;
        case P_W4_CONNECT_CANCELLED: printf("P_W4_CONNECT_CANCELLED"); break;
        case P_W2_DISCONNECT: printf("P_W2_DISCONNECT"); break;
        case P_W4_DISCONNECTED: printf("P_W4_DISCONNECTED"); break;
        case P_W2_SEND_SERVICE_QUERY: printf("P_W2_SEND_SERVICE_QUERY"); break;
        case P_W4_SERVICE_QUERY_RESULT:printf("P_W4_SERVICE_QUERY_RESULT"); break;
        case P_W2_SEND_CHARACTERISTIC_QUERY: printf("P_W2_SEND_CHARACTERISTIC_QUERY"); break;
        case P_W4_CHARACTERISTIC_QUERY_RESULT:printf("P_W4_CHARACTERISTIC_QUERY_RESULT"); break;
        case P_W2_SEND_INCLUDE_SERVICE_QUERY:printf("P_W2_SEND_INCLUDE_SERVICE_QUERY"); break;
        case P_W4_INCLUDE_SERVICE_QUERY_RESULT:printf("P_W4_INCLUDE_SERVICE_QUERY_RESULT"); break;
    };
    printf("\n");
}

void printUUID128(const uint8_t * uuid){
    int i;
    for (i=15; i >= 0 ; i--){
        printf("%02X", uuid[i]);
        switch (i){
            case 4:
            case 6:
            case 8:
            case 10:
                printf("-");
                break;
            default:
                break;
        }
    }
}

typedef enum {
    TC_IDLE,
    TC_W4_SCAN_RESULT,
    TC_SCAN_ACTIVE,
    TC_W2_CONNECT,
    TC_W4_CONNECT,

    TC_W2_SERVICE_REQUEST,
    TC_W4_SERVICE_RESULT,
    
    TC_W2_CHARACTERISTIC_REQUEST,
    TC_W4_CHARACTERISTIC_RESULT,
    
    TC_W2_DISCONNECT,
    TC_W4_DISCONNECT,
    TC_DISCONNECTED

} tc_state_t;

tc_state_t tc_state = TC_IDLE;

void test_client(){
    le_command_status_t status;
    //dump_state();
    dump_peripheral_state(test_device.state);

    switch(tc_state){
        case TC_IDLE: 
            // printf("test client - TC_IDLE\n");
            status = le_central_start_scan(); 
            if (status != BLE_PERIPHERAL_OK) return;
            tc_state = TC_W4_SCAN_RESULT;
            // printf("test client - TC_W4_SCAN_RESULT\n");
            break;

        case TC_SCAN_ACTIVE: 
            le_central_stop_scan();
            status = le_central_connect(&test_device, 0, test_device_addr);
            if (status != BLE_PERIPHERAL_OK) return;
            tc_state = TC_W4_CONNECT;
            break;

        case TC_W2_SERVICE_REQUEST: 
            status = le_central_get_services(&test_device);
            if (status != BLE_PERIPHERAL_OK) return;
            tc_state = TC_W4_SERVICE_RESULT;
            break;

        case TC_W2_CHARACTERISTIC_REQUEST:
            if (service_index >= service_count) tc_state = TC_W2_DISCONNECT;

            printf("test client - TC_W2_CHARACTERISTIC_REQUEST, for service %02x [%d/%d]\n", services[service_index].uuid16, service_index+1, service_count);

            status = le_central_get_characteristics_for_service(&test_device, &services[service_index]);
            if (status != BLE_PERIPHERAL_OK) return;
            service_index++;
            tc_state = TC_W4_CHARACTERISTIC_RESULT;
            printf("le_central_get_characteristics_for_service requested\n");
            break;
            
        case TC_W2_DISCONNECT:
            status = le_central_disconnect(&test_device);
            if (status != BLE_PERIPHERAL_OK) return;
            tc_state = TC_W4_DISCONNECT;

        default: 
            break;

    }
}

static void handle_le_central_event(le_central_event_t * event){
    ad_event_t * advertisement_event;
    le_peripheral_event_t * peripheral_event;
    le_query_complete_event_t * query_event;
    
    le_service_t service;
    le_characteristic_t characteristic;

    switch (event->type){
        case GATT_ADVERTISEMENT:
            if (tc_state != TC_W4_SCAN_RESULT) return;
            
            advertisement_event = (ad_event_t*) event;
            dump_ad_event(advertisement_event);
            tc_state = TC_SCAN_ACTIVE;
            printf("test client - TC_SCAN_ACTIVE\n");
            
            break;
        case GATT_CONNECTION_COMPLETE:
            printf("test client - GATT_CONNECTION_COMPLETE\n");
            // if (tc_state != TC_W4_CONNECT || tc_state != TC_W4_DISCONNECT) return;

            peripheral_event = (le_peripheral_event_t *) event;
            if (peripheral_event->status == 0){
                // printf("handle_le_central_event::device is connected\n");
                tc_state = TC_W2_SERVICE_REQUEST;
                printf("test client - TC_CONNECTED\n");
                test_client();
            } else {
                printf("handle_le_central_event::disconnected with status %02x \n", peripheral_event->status);
                tc_state = TC_DISCONNECTED;
                printf("test client - TC_DISCONNECTED\n");
            }
            break;

        case GATT_SERVICE_QUERY_RESULT:
            service = ((le_service_event_t *) event)->service;
            services[service_count++] = service;
            test_client();
            break;

        case GATT_SERVICE_QUERY_COMPLETE:
            query_event = (le_query_complete_event_t*) event;
            tc_state = TC_W2_CHARACTERISTIC_REQUEST;
            int i;
            for (i = 0; i < service_count; i++){
                printf("    *** found service ***  uuid %02x, start group handle %02x, end group handle %02x", 
                    services[i].uuid16, services[i].start_group_handle, services[i].end_group_handle);
                printf("\n");
            }
            test_client();
            
            break;
        
        case GATT_CHARACTERISTIC_QUERY_RESULT:
            characteristic = ((le_characteristic_event_t *) event)->characteristic;
            printf("    *** found characteristic *** properties %x, handle %02x, uuid ", characteristic.properties, characteristic.value_handle);
            printUUID128(characteristic.uuid128);
            printf("\n");
            test_client();
            break;

        case GATT_CHARACTERISTIC_QUERY_COMPLETE:
            query_event = (le_query_complete_event_t*) event;
            if (service_index == service_count) {
                tc_state = TC_W2_DISCONNECT; 
                break;
            }
            tc_state = TC_W2_CHARACTERISTIC_REQUEST;
            test_client();
            
            break;
        default:
            break;
    }
}


void connect_to_ancs()
{
    le_central_register_handler(handle_le_central_event);

}