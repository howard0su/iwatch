
#include "stlv_handler.h"

#include <stdio.h>
#include "stlv.h"
#include "contiki.h"
#include "window.h"
#include "rtc.h"
#include "cfs/cfs.h"
#include "stlv_client.h"

static void print_stlv_string(unsigned char* data, int len)
{
    unsigned char back = data[len];
    data[len] = '\0';
    printf((char*)data);
    data[len] = back;
}

static void handle_msg_element(uint8_t msg_type, stlv_packet pack, element_handle handle)
{
    element_handle begin = get_first_sub_element(pack, handle);
    char filter[2] = { SUB_TYPE_MESSAGE_IDENTITY, SUB_TYPE_MESSAGE_MESSAGE, };
    element_handle sub_handles[2] = {0};
    int sub_element_count = filter_elements(pack, handle, begin, filter, 2, sub_handles);
    if (sub_element_count != 0x03)
    {
        printf("Cannot find expected sub-elements: %c, %c\n", filter[0], filter[1]);
        return;
    }

    int identity_len = get_element_data_size(pack, sub_handles[0], NULL, 0);
    unsigned char* identity_data = get_element_data_buffer(pack, sub_handles[0], NULL, 0);

    int message_len = get_element_data_size(pack, sub_handles[1], NULL, 0);
    unsigned char* message_data = get_element_data_buffer(pack, sub_handles[1], NULL, 0);

    identity_data[identity_len] = '\0';
    message_data[message_len] = '\0';
    printf("From: %s\n", identity_data);
    printf("Message: %s\n", message_data);
    handle_message(msg_type, (char*)identity_data, (char*)message_data);

}


void handle_echo(uint8_t* data, int data_len)
{
    send_echo(data, data_len);
}

void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_setdate(2000 + year, month, day);
    rtc_settime(hour, minute, second);
}

#define ICON_FACEBOOK 's'
#define ICON_TWITTER  't'
#define ICON_MSG      'u'

void handle_message(uint8_t msg_type, char* ident, char* message)
{
    uint8_t icon;
    switch (msg_type)
    {
    case ELEMENT_TYPE_MESSAGE_SMS:
        icon = ICON_MSG;
        break;
    case ELEMENT_TYPE_MESSAGE_FB:
        icon = ICON_FACEBOOK;
        break;
    case ELEMENT_TYPE_MESSAGE_TW:
        icon = ICON_TWITTER;
        break;
    default:
        return;
        break;
    }

    window_notify(ident, message, NOTIFY_OK, icon);

}

int handle_file_begin(char* name)
{
    int fd = cfs_open(name, CFS_WRITE);
    if (fd == -1)
    {
        printf("Open file %s failed\n", name);
    }

    return fd;
}

int handle_file_data(int fd, uint8_t* data, uint8_t size)
{
    if (fd != -1)
    {
        return cfs_write(fd, data, size);
    }

    return 0;
}

void handle_file_end(int fd)
{
    cfs_close(fd);
}


