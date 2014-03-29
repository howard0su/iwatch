
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

//XX: we should not use window.h here
#include "window.h"

static const uint8_t ancsuuid[] = {
    0x79, 0x05, 0xF4, 0x31, 0xB5, 0xCE, 0x4E, 0x99, 0xA4, 0x0F, 0x4B, 0x1E, 0x12, 0x2D, 0x00, 0xD0
};
static const uint8_t notificationuuid[] = {
    0x9F, 0xBF, 0x12, 0x0D, 0x63, 0x01, 0x42, 0xD9, 0x8C, 0x58, 0x25, 0xE6, 0x99, 0xA2, 0x1D, 0xBD
};
static const uint8_t controluuid[] = {
    0x69, 0xD1, 0xD8, 0xF3, 0x45, 0xE1, 0x49, 0xA8, 0x98, 0x21, 0x9B, 0xBD, 0xFD, 0xAA, 0xD9, 0xD9
};
static const uint8_t datauuid[] = {
    0x22, 0xEA, 0xC6, 0xE9, 0x24, 0xD6, 0x4B, 0xB5, 0xBE, 0x44, 0xB3, 0x6A, 0xCE, 0x7C, 0x7B, 0xFB
};
static const uint8_t* attribute_uuids[] =
{
    notificationuuid, controluuid, datauuid
};
static uint16_t start_group_handle, end_group_handle;

#define NOTIFICATION 0
#define CONTROLPOINT 1
#define DATASOURCE   2
static uint16_t attribute_handles[3];

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
            for(int i = 0 ; i < 3; i++)
                attribute_handles[i] = 0xffff;
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

        for(int i = 0 ;i < 3; i++)
        {
            if (memcmp(uuid128, attribute_uuids[i], 16) == 0)
            {
                attribute_handles[i] = value_handle;
            }
        }

    }

    if (attribute_handles[0] != -1 && 
        attribute_handles[1] != -1 &&
        attribute_handles[2] != -1)
    {
        // subscribe event
        printf("sub to %d\n", attribute_handles[NOTIFICATION]);
        att_server_subscribe(attribute_handles[NOTIFICATION] + 1); // write to CCC
        return 0xffff;
    }
    else
        return value_handle;
}

void report_write_done(att_connection_t *conn, uint16_t handle)
{
    printf("report_write_done: %d\n", handle);
    if (handle == attribute_handles[NOTIFICATION] + 1)
    {
        att_server_subscribe(attribute_handles[DATASOURCE] + 1); // write to CCC
    }
}

#define MAX_TITLE 16
#define MAX_MESSAGE 256
static enum 
{
    STATE_NONE,
    STATE_UID,
    STATE_ATTRIBUTEID,
    STATE_ATTRIBUTELEN,
    STATE_ATTRIBUTE,
    STATE_DONE
}parse_state = STATE_NONE;
static uint8_t attributeid;
static uint16_t attrleftlen, len;
static char* bufptr;
static char appidbuf[32];
static char titlebuf[MAX_TITLE + 1];
static char msgbuf[MAX_MESSAGE + 1];

void att_client_notify(uint16_t handle, uint8_t *data, uint16_t length)
{
    if (handle == attribute_handles[NOTIFICATION])
    {
        uint32_t uid =  READ_BT_32(data, 4);
        printf("id: %d flags:%d catery:%d count: %d UID:%ld\n",
            data[0], data[1], data[2], data[3],
            uid
            );

        if (data[0] != 0)
            return;

        // based on catery to fetch infomation
        uint8_t buffer[] = {
            0, // command id
            0, 0, 0, 0, // uid
            0,          // appid
            1, MAX_TITLE, 0, // 16 bytes title
            3, MAX_MESSAGE, 0, // 64bytes message
        };
        bt_store_32(buffer, 1, uid);
        att_server_write(attribute_handles[CONTROLPOINT], buffer, sizeof(buffer));
    }
    else if (handle == attribute_handles[DATASOURCE])
    {
        printf("data received\n");
        // start notification

        int index = 0;
        while(index < length)
        {
            switch(parse_state)
            {
                case STATE_NONE:
                    printf("Command: %d\t", data[index]);
                    index++;
                    parse_state = STATE_UID;
                    break;
                case STATE_UID:
                    printf("uid: %ld\t", READ_BT_32(data, index));
                    index += 4;
                    parse_state = STATE_ATTRIBUTEID;
                    break;
                case STATE_ATTRIBUTEID:
                    attributeid = data[index];
                    printf("\nattributeid: %d\t", attributeid);
                    switch(attributeid)
                    {
                        case 0:
                        bufptr = appidbuf;
                        break;
                        case 1:
                        bufptr = titlebuf;
                        break;
                        case 3:
                        bufptr = msgbuf;
                        break;
                    }
                    index++;
                    parse_state = STATE_ATTRIBUTELEN;
                    break;
                case STATE_ATTRIBUTELEN:
                    len = attrleftlen = READ_BT_16(data, index);
                    printf("len: %d\t", attrleftlen);
                    index+=2;
                    parse_state = STATE_ATTRIBUTE;
                    break;
                case STATE_ATTRIBUTE:
                    uint16_t l;
                    if (length - index > attrleftlen)
                        l = attrleftlen;
                    else
                        l = length - index;
                    for(int i = 0; i < l; i++)
                    {
                        putchar(data[index + i]);
                        bufptr[i + len - attrleftlen] = data[index + i];
                    }
                    index += l;
                    attrleftlen -= l;
                    if (attrleftlen == 0)
                    {
                        bufptr[len] = '\0';
                        if (attributeid == 3)
                            parse_state = STATE_NONE;
                        else
                            parse_state = STATE_ATTRIBUTEID;
                    }
                    break;
            }
        }
        // parse the data

        if (parse_state == STATE_NONE)
        {
            window_notify(titlebuf, msgbuf, 0, -1);
        }
    }
    else
    {
        printf("handle: %d\n", handle);
    }
}