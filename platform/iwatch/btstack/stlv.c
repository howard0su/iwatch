
#include "stlv.h"
#include <stdio.h>

#define GET_PACKET_END(pack) (pack + STLV_HEAD_SIZE + get_body_length(pack))

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
    int parent_head_len = get_element_type(pack, parent, 0, 0);
    int parent_body_len = get_element_data_size(pack, parent, NULL, 0);

    int element_head_len = get_element_type(pack, handle, 0, 0);
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
            buf[cursor++] = (*ptr++ & ~0x80);
    }
    if (cursor < buf_size && buf != NULL)
        buf[cursor++] = (*ptr++ & ~0x80);
    return cursor;
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
                printf("echo: %s\n", (char*)data);
            }
            break;
            
        case ELEMENT_TYPE_CLOCK:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                unsigned char* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                printf("clock: %d/%d/%d %d:%d:%d\n", 
                    (int)data[0], (int)data[1], (int)data[2], (int)data[3], (int)data[4], (int)data[5]);
            }
            break;
            
        case ELEMENT_TYPE_MESSAGE:
            if (type_len == 2)
            {
                switch (type_buf[1])
                {
                case ELEMENT_TYPE_MESSAGE_SMS:
                    printf("Notification: SMS:\n");
                case ELEMENT_TYPE_MESSAGE_FB:
                    printf("Notification: Facebook:\n");
                case ELEMENT_TYPE_MESSAGE_TW:
                    printf("Notification: Twitter:\n");
                    {
                        element_handle begin = get_first_sub_element(pack, handle);
                        char filter[2] = { SUB_TYPE_MESSAGE_IDENTITY, SUB_TYPE_MESSAGE_MESSAGE, };
                        element_handle sub_handles[2] = {0};
                        int sub_element_count = filter_elements(pack, handle, begin, filter, 2, sub_handles);
                        if (sub_element_count != 0x03)
                            break;

                        printf("From    : %s\n", get_element_data_buffer(pack, sub_handles[0], NULL, 0));
                        printf("Content : %s\n", get_element_data_buffer(pack, sub_handles[1], NULL, 0));
                    }
                default:
                    break;
                }
                
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

