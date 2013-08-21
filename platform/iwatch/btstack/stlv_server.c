
#include "stlv_server.h"

#include <stdio.h>
#include "stlv.h"
#include "contiki.h"
#include "window.h"
#include "rtc.h"
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

//TODO: implement these 3 file data functions
void handle_file_begin(char* name)
{
}

void handle_file_data(char* name, uint8_t* data, uint8_t size)
{
}

void handle_file_end(char* name)
{
}


