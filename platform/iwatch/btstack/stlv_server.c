
#include "stlv_server.h"

#include <stdio.h>
#include "stlv.h"
#include "stlv_handler.h"

static void handle_file(stlv_packet pack, element_handle handle);
static void print_stlv_string(unsigned char* data, int len);
static void handle_msg_element(uint8_t msg_type, stlv_packet pack, element_handle handle);
static void handle_gps_data(stlv_packet pack, element_handle handle);

void handle_stlv_packet(unsigned char* packet)
{
    stlv_packet pack = packet;
    char type_buf[MAX_ELEMENT_TYPE_BUFSIZE];

    element_handle handle = get_first_element(pack);
    while (IS_VALID_STLV_HANDLE(handle))
    {
        int type_len = get_element_type(pack, handle, type_buf, sizeof(type_buf));
        printf("Read Element: %x\n", type_buf[0]);
        switch (type_buf[0])
        {
        case ELEMENT_TYPE_ECHO:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                unsigned char* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                printf("echo: ");
                print_stlv_string(data, data_len);
                printf("\n");
                handle_echo(data, data_len);
            }
            break;

        case ELEMENT_TYPE_CLOCK:
            {
                unsigned char* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                printf("clock: %d/%d/%d %d:%d:%d\n",
                    (int)data[0], (int)data[1], (int)data[2], (int)data[3], (int)data[4], (int)data[5]);
                handle_clock(data[0], data[1], data[2], data[3], data[4], data[5]);
            }
            break;

        case ELEMENT_TYPE_MESSAGE:
            if (type_len == 2)
            {
                switch (type_buf[1])
                {
                case ELEMENT_TYPE_MESSAGE_SMS:
                    printf("notification(SMS):\n");
                    break;
                case ELEMENT_TYPE_MESSAGE_FB:
                    printf("notification(Facebook):\n");
                    break;
                case ELEMENT_TYPE_MESSAGE_TW:
                    printf("notification(Twitter):\n");
                    break;
                default:
                    break;
                }
                handle_msg_element(type_buf[1], pack, handle);
            }
            break;

        case ELEMENT_TYPE_FILE:
            handle_file(pack, handle);
            break;

        case ELEMENT_TYPE_GET_FILE:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                STLV_BUF_BEGIN_TEMP_STRING(data, data_len);
                handle_get_file((char*)data);
                STLV_BUF_END_TEMP_STRING(data, data_len);
            }
            break;

        case ELEMENT_TYPE_LIST_FILES:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                STLV_BUF_BEGIN_TEMP_STRING(data, data_len);
                handle_list_file((char*)data);
                STLV_BUF_END_TEMP_STRING(data, data_len);
            }
            break;

        case ELEMENT_TYPE_REMOVE_FILE:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                STLV_BUF_BEGIN_TEMP_STRING(data, data_len);
                uint8_t file_name_pos = 0;
                for (uint8_t i = 0; i < data_len; ++i)
                {
                    if (data[i] == ';')
                        data[i] = '\0';

                    if (data[i] == '\0')
                    {
                        handle_remove_file((char*)(&data[file_name_pos]));
                        file_name_pos = i + 1;
                    }
                }
                STLV_BUF_END_TEMP_STRING(data, data_len);
            }
           break;

#if 0
        case ELEMENT_TYPE_SPORT_HEARTBEAT:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                STLV_BUF_BEGIN_TEMP_STRING(data, data_len);
                handle_sports_heartbeat((char*)data);
                STLV_BUF_END_TEMP_STRING(data, data_len);
            }
            break;
        case ELEMENT_TYPE_SPORTS_DATA:
            handle_get_sports_data();
            break;
#endif
        case ELEMENT_TYPE_SPORTS_GRID:
            printf("Get Sports Grid Request\n");
            handle_get_sports_grid();
            break;

        case ELEMENT_TYPE_ALARM:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                if (data_len != sizeof(alarm_conf_t))
                {
                    printf("Alarm element decode failed: length mismatch (%d/%d)", data_len, sizeof(alarm_conf_t));
                }
                else
                {
                    handle_alarm((alarm_conf_t*)data);
                }
            }
            break;

        case ELEMENT_TYPE_SN:
            handle_get_device_id();
            break;

        case ELEMENT_TYPE_ACTIVITY:
            handle_gps_data(pack, handle);
            break;

        case ELEMENT_TYPE_GESTURE_CONTROL:
            {
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                if (data_len != 5)
                {
                    printf("gesture control decode failed: length mismatch (%d/1)", data_len);
                }
                else
                {
                    handle_gesture_control(*data, data + 1);
                }
            }
            break;

        case ELEMENT_TYPE_WATCHCONFIG:
            {
                printf("Set Watch UI Config");
                int data_len = get_element_data_size(pack, handle, type_buf, type_len);
                uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                UNUSED_VAR(data_len);
                handle_set_watch_config((ui_config*)data);
            }
            break;

        }



        handle = get_next_element(pack, handle);

    }
}

static void handle_file(stlv_packet pack, element_handle handle)
{
    static int s_file_handle = -1;

    char type_buf[MAX_ELEMENT_TYPE_BUFSIZE];

    element_handle element = get_first_sub_element(pack, handle);
    while (IS_VALID_STLV_HANDLE(element))
    {
        int type_len = get_element_type(pack, element, type_buf, sizeof(type_buf));
        switch (type_buf[0])
        {
            case SUB_TYPE_FILE_NAME:
                {
                    uint8_t* file_name_data = get_element_data_buffer(pack, element, type_buf, type_len);
                    uint8_t  file_name_size = get_element_data_size(pack, element, type_buf, type_len);
                    uint8_t  temp = file_name_data[file_name_size];
                    file_name_data[file_name_size] = '\0';
                    char* file_name = (char*)file_name_data;
                    s_file_handle = handle_file_begin(file_name);
                    printf("handle_file_begin(%s) return %d\n", file_name, s_file_handle);
                    file_name_data[file_name_size] = temp;
                }
                break;

            case SUB_TYPE_FILE_DATA:
                {
                    uint8_t* file_data = get_element_data_buffer(pack, element, type_buf, type_len);
                    uint8_t  data_size = get_element_data_size(pack, element, type_buf, type_len);
                    printf("handle_file_data(fd=%d, %d bytes)\n", s_file_handle, data_size);
                    handle_file_data(s_file_handle, file_data, data_size);
                }
                break;

            case SUB_TYPE_FILE_END:
                printf("handle_file_end(fd=%d)\n", s_file_handle);
                handle_file_end(s_file_handle);
                s_file_handle = 0;
                break;
        }
        element = get_next_sub_element(pack, handle, element);
    }
}

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

static void handle_gps_data(stlv_packet pack, element_handle handle)
{
    char type_buf[MAX_ELEMENT_TYPE_BUFSIZE];

    uint16_t gps_spd = 0;
    uint16_t gps_alt = 0;
    uint32_t gps_dis = 0;

    element_handle element = get_first_sub_element(pack, handle);
    while (IS_VALID_STLV_HANDLE(element))
    {
        int type_len = get_element_type(pack, element, type_buf, sizeof(type_buf));
        switch (type_buf[0])
        {
            case SUB_TYPE_ACTIVITY_ALT:
                {
                    uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                    gps_alt = *((uint16_t*)data);
                }
                break;

            case SUB_TYPE_ACTIVITY_SPD:
                {
                    uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                    gps_spd = *((uint16_t*)data);
                }
                break;

            case SUB_TYPE_ACTIVITY_DIS:
                {
                    uint8_t* data = get_element_data_buffer(pack, handle, type_buf, type_len);
                    gps_dis = *((uint32_t*)data);
                }
               break;
        }
        element = get_next_sub_element(pack, handle, element);
    }

    handle_gps_info(gps_spd, gps_alt, gps_dis, 0);

}
