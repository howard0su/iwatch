
#include "stlv_handler.h"

#include <stdio.h>
#include "stlv.h"
#include "contiki.h"
#include "window.h"
#include "rtc.h"
#include "cfs/cfs.h"
#include "stlv_client.h"

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

//TODO: sujun implement this heart beat and get file
void handle_sports_heartbeat(uint8_t seconds_to_next)
{
}

void handle_get_file(char* name)
{
}


