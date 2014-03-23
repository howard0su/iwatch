
#include "att_client.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack-config.h"

#include <btstack/run_loop.h>
#include "debug.h"
#include "btstack/sdp_util.h"
#include "btstack_memory.h"
#include "hci.h"
#include "hci_dump.h"

#include "l2cap.h"

#include "sm.h"
#include "att.h"
#include "att_server.h"
#include "gap_le.h"
#include "central_device_db.h"

typedef enum
{
    P_W2_SEND_SERVICE_QUERY,
    P_W4_SERVICE_QUERY_RESULT,
    P_W2_SEND_SERVICE_WITH_UUID_QUERY,
    P_W4_SERVICE_WITH_UUID_RESULT,
    
    P_W2_SEND_CHARACTERISTIC_QUERY,
    P_W4_CHARACTERISTIC_QUERY_RESULT,
    P_W2_SEND_CHARACTERISTIC_WITH_UUID_QUERY,
    P_W4_CHARACTERISTIC_WITH_UUID_QUERY_RESULT,
    P_W2_SEND_CHARACTERISTIC_DESCRIPTOR_QUERY,
    P_W4_CHARACTERISTIC_DESCRIPTOR_QUERY_RESULT,

    P_W2_SEND_INCLUDED_SERVICE_QUERY,
    P_W4_INCLUDED_SERVICE_QUERY_RESULT,
    P_W2_SEND_INCLUDED_SERVICE_WITH_UUID_QUERY,
    P_W4_INCLUDED_SERVICE_UUID_WITH_QUERY_RESULT,
    
    P_W2_SEND_READ_CHARACTERISTIC_VALUE_QUERY,
    P_W4_READ_CHARACTERISTIC_VALUE_RESULT,

    P_W2_SEND_READ_LONG_CHARACTERISTIC_VALUE_QUERY,
    P_W4_READ_LONG_CHARACTERISTIC_VALUE_RESULT
}state_t;

uint16_t report_gatt_services(att_connection_t *conn, uint8_t * packet,  uint16_t size){
    // printf(" report_gatt_services for %02X\n", peripheral->handle);
    uint8_t attr_length = packet[1];
    uint16_t last = 0;
    uint8_t uuid_length = attr_length - 4;
    
    le_service_t service;
    le_service_event_t event;
    event.type = GATT_SERVICE_QUERY_RESULT;
    
    int i;
    for (i = 2; i < size; i += attr_length){
        service.start_group_handle = READ_BT_16(packet,i);
        service.end_group_handle = READ_BT_16(packet,i+2);
        
        if (uuid_length == 2){
            service.uuid16 = READ_BT_16(packet, i+4);
            sdp_normalize_uuid((uint8_t*) &service.uuid128, service.uuid16);
        } else {
            swap128(&packet[i+4], service.uuid128);
        }
        event.service = service;
        printUUID128(service.uuid128);
        printf("\nstart_group_handle: %d, end_group_handle: %d\n", service.start_group_handle, service.end_group_handle);
        last = service.end_group_handle;
    }

    return last;
}

