
#include "att_client.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack-config.h"

#include <btstack/run_loop.h>
#include <btstack/utils.h>
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
    SERVICE_QUERY_RESULT,
    CHARACTERISTIC_QUERY_RESULT,
    
    P_W2_SEND_READ_CHARACTERISTIC_VALUE_QUERY,
    P_W4_READ_CHARACTERISTIC_VALUE_RESULT,

    P_W2_SEND_READ_LONG_CHARACTERISTIC_VALUE_QUERY,
    P_W4_READ_LONG_CHARACTERISTIC_VALUE_RESULT
}state_t;

static const uint8_t ancsuuid[] = {
    0x79, 0x05, 0xF4, 0x31, 0xB5, 0xCE, 0x4E, 0x99, 0xA4, 0x0F, 0x4B, 0x1E, 0x12, 0x2D, 0x00, 0xD0
};

static uint16_t start_group_handle, end_group_handle;

uint16_t report_gatt_services(att_connection_t *conn, uint8_t * packet,  uint16_t size){
    // printf(" report_gatt_services for %02X\n", peripheral->handle);
    uint8_t attr_length = packet[1];
    uint16_t last = 0;
    uint8_t uuid_length = attr_length - 4;
    uint8_t uuid128[16];

    int i;
    for (i = 2; i < size; i += attr_length){
        start_group_handle = READ_BT_16(packet,i);
        end_group_handle = READ_BT_16(packet,i+2);
        
        if (uuid_length == 2){
            uint16_t uuid16 = READ_BT_16(packet, i+4);
            sdp_normalize_uuid((uint8_t*) &uuid128, uuid16);
        } else {
            swap128(&packet[i+4], uuid128);
        }
        printUUID128(uuid128);
        printf("\nstart_group_handle: %d, end_group_handle: %d\n", start_group_handle, end_group_handle);

        if (memcmp(ancsuuid, uuid128, 16) == 0)
        {
            att_server_read_gatt_service(start_group_handle, end_group_handle);
            return 0xff;
        }
    }

    return end_group_handle;
}

uint16_t report_service_characters(att_connection_t *conn, uint8_t * packet,  uint16_t size){
    uint8_t attr_length = packet[1];
    uint8_t uuid_length = attr_length - 5;
    uint16_t value_handle;
    int i;        
    for (i = 2; i < size; i += attr_length){
        uint16_t start_handle = READ_BT_16(packet, i);
        uint8_t  properties = packet[i+2];
        value_handle = READ_BT_16(packet, i+3);

        uint8_t uuid128[16];
        if (uuid_length == 2){
            uint16_t uuid16 = READ_BT_16(packet, i+5);
            sdp_normalize_uuid((uint8_t*) &uuid128, uuid16);
        } else {
            swap128(&packet[i+5], uuid128);
        }

        printUUID128(uuid128);
        printf("\nproperties: %x start_handle:%d value_handle: %d\n", properties, start_handle, value_handle);
    }

    return value_handle;
}