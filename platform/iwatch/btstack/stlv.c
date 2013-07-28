#include "contiki.h"
#include "window.h"
#include "stlv.h"
#include <stdio.h>
#include "rtc.h"

#define GET_PACKET_END(pack) (pack + STLV_HEAD_SIZE + get_body_length(pack))

#define ICON_FACEBOOK 's'
#define ICON_TWITTER  't'
#define ICON_MSG      'u'


int get_version(stlv_packet pack)
{
    return pack[HEADFIELD_VERSION];
}

int get_body_length(stlv_packet pack)
{
    return pack[HEADFIELD_BODY_LENGTH];
}

int get_sequence(stlv_packet pack)
{
    return pack[HEADFIELD_SEQUENCE];
}

int get_flag(stlv_packet pack)
{
    return pack[HEADFIELD_FLAG];
}

element_handle get_first_element(stlv_packet pack)
{
    if (get_body_length(pack) >= MIN_ELEMENT_SIZE)
        return pack + STLV_HEAD_SIZE;
    else
        return STLV_INVALID_HANDLE;
}

element_handle get_next_element(stlv_packet pack, element_handle handle)
{
    int len = get_element_data_size(pack, handle, 0, 0);
    if (GET_PACKET_END(pack) - (handle + len) >= MIN_ELEMENT_SIZE)
        return handle + len;
    else
        return STLV_INVALID_HANDLE;
}

element_handle get_first_sub_element(stlv_packet pack, element_handle parent)
{
    return get_element_data_buffer(pack, parent, 0, 0);
}

element_handle get_next_sub_element(stlv_packet pack, element_handle parent, element_handle handle)
{
    int parent_head_len = get_element_type(pack, parent, 0, 0) + 1;
    int parent_body_len = get_element_data_size(pack, parent, NULL, 0);

    int element_head_len = get_element_type(pack, handle, 0, 0) + 1;
    int element_body_len = get_element_data_size(pack, handle, NULL, 0);

    element_handle parent_end  = parent + parent_head_len  + parent_body_len;
    element_handle element_end = handle + element_head_len + element_body_len;

    if (parent_end - element_end < MIN_ELEMENT_SIZE)
        return STLV_INVALID_HANDLE;
    else
        return element_end;
    
}

int get_element_type(stlv_packet pack, element_handle handle, char* buf, int buf_size)
{
    int cursor = 0;
    unsigned char* ptr = handle;
    while ((*ptr & 0x80) != 0)
    {
        if (cursor < buf_size && buf != NULL)
            buf[cursor] = (*ptr & ~0x80);
        ptr++;
        cursor++;
    }
    if (cursor < buf_size && buf != NULL)
        buf[cursor] = (*ptr & ~0x80);

    return cursor + 1;
}

int get_element_data_size(stlv_packet pack, element_handle handle, char* buf, int buf_size)
{
    if (buf != NULL && buf_size != 0)
        return handle[buf_size];
    else
        return handle[get_element_type(pack, handle, buf, buf_size)];
}

unsigned char* get_element_data_buffer(stlv_packet pack, element_handle handle, char* buf, int buf_size)
{
    if (buf != NULL && buf_size != 0)
        return handle + buf_size + 1;
    else
        return handle + get_element_type(pack, handle, buf, buf_size) + 1;
}

static void print_stlv_string(unsigned char* data, int len)
{
    unsigned char back = data[len];
    data[len] = '\0';
    printf((char*)data);
    data[len] = back;
}

static void handle_msg_element(uint8_t icon, stlv_packet pack, element_handle handle)
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
    window_notify(identity_data,
                  message_data,
                  NOTIFY_OK,
                  icon
                  );
}
   
void handle_stlv_packet(unsigned char* packet)
{
    stlv_packet pack = packet;
    char type_buf[MAX_ELEMENT_TYPE_BUFSIZE];

    element_handle handle = get_first_element(pack);
    while (IS_VALID_STLV_HANDLE(handle))
    {
        int type_len = get_element_type(pack, handle, type_buf, sizeof(type_buf));
        switch (type_buf[0])
        {
        case ELEMENT_TYPE_ECHO:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                unsigned char* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                printf("echo: ");
                print_stlv_string(data, data_len);
                printf("\n");
            }
            break;
            
        case ELEMENT_TYPE_CLOCK:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                unsigned char* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                printf("clock: %d/%d/%d %d:%d:%d\n", 
                    (int)data[0], (int)data[1], (int)data[2], (int)data[3], (int)data[4], (int)data[5]);
                rtc_setdate(2000 + data[0], data[1], data[2]);
                rtc_settime(data[3], data[4], data[5]);
            }
            break;
            
        case ELEMENT_TYPE_MESSAGE:
            if (type_len == 2)
            {
                uint8_t icon;
                switch (type_buf[1])
                {
                case ELEMENT_TYPE_MESSAGE_SMS:
                    printf("notification(SMS):\n");
                    icon = ICON_MSG;
                    break;
                case ELEMENT_TYPE_MESSAGE_FB:
                    printf("notification(Facebook):\n");
                    icon = ICON_FACEBOOK;
                    break;
                case ELEMENT_TYPE_MESSAGE_TW:
                    printf("notification(Twitter):\n");
                    icon = ICON_TWITTER;
                    break;
                default:
                    break;
                }
                handle_msg_element(icon, pack, handle);                
            }
            break;
            
        }
        
        handle = get_next_element(pack, handle);
        
    }
}

int filter_elements(
    stlv_packet pack, 
    element_handle parent, 
    element_handle begin, 
    char* filter, 
    int filter_size, 
    element_handle* result
    )
{
    char type_buf[MAX_ELEMENT_TYPE_BUFSIZE];
    element_handle handle = begin;
    int mask  = 0;
    while (IS_VALID_STLV_HANDLE(handle))
    {
        int type_len = get_element_type(pack, handle, type_buf, sizeof(type_buf));
        for (int i = 0; i < filter_size; ++i)
        {
            if (filter[i] == type_buf[0])
            {
                result[i] = handle;
                mask |= (1 << i);
                break;
            }
        }

        if (IS_VALID_STLV_HANDLE(parent))
            handle = get_next_sub_element(pack, parent, handle);
        else
            handle = get_next_element(pack, handle);
    }
    return mask;
}

