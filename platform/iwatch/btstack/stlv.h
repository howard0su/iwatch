
#ifndef _STLV_H_
#define _STLV_H_

#include <stdint.h>

#define UNUSED_VAR(a) a = a
#define STLV_INVALID_HANDLE 0
#define IS_VALID_STLV_HANDLE(h) (h != STLV_INVALID_HANDLE)

#define STLV_PACKET_MAX_BODY_SIZE 240
#define STLV_HEAD_SIZE   4
#define STLV_PACKET_MAX_SIZE      (STLV_PACKET_MAX_BODY_SIZE + STLV_HEAD_SIZE)

#define MAX_ELEMENT_NESTED_LAYER  4
#define MIN_ELEMENT_SIZE 2
#define MAX_ELEMENT_TYPE_SIZE    3
#define MAX_ELEMENT_TYPE_BUFSIZE (MAX_ELEMENT_TYPE_SIZE + 1)

#define HEADFIELD_VERSION     0
#define HEADFIELD_FLAG        1
#define HEADFIELD_BODY_LENGTH 2
#define HEADFIELD_SEQUENCE    3

#define ELEMENT_TYPE_CLOCK            'C'
#define ELEMENT_TYPE_ECHO             'E'
#define ELEMENT_TYPE_SPORT_HEARTBEAT  'H'
#define ELEMENT_TYPE_GET_FILE         'G'
#define ELEMENT_TYPE_LIST_FILES       'L'
#define ELEMENT_TYPE_REMOVE_FILE      'X'
#define ELEMENT_TYPE_SPORTS_DATA      'A'
#define ELEMENT_TYPE_SPORTS_GRID      'R'
#define ELEMENT_TYPE_SN               'S'
#define ELEMENT_TYPE_WATCHFACE        'W'

#define ELEMENT_TYPE_ALARM            'I'
#define     SUB_TYPE_ALARM_OPERATION  'o'
#define     SUB_TYPE_ALARM_VALUE      'd'

#define ELEMENT_TYPE_FILE             'F'
#define     SUB_TYPE_FILE_NAME        'n'
#define     SUB_TYPE_FILE_DATA        'd'
#define     SUB_TYPE_FILE_END         'e'

#define ELEMENT_TYPE_MESSAGE              'M'
#define     ELEMENT_TYPE_MESSAGE_SMS      'S'
#define     ELEMENT_TYPE_MESSAGE_FB       'F'
#define     ELEMENT_TYPE_MESSAGE_TW       'T'
#define         SUB_TYPE_MESSAGE_IDENTITY 'i'
#define         SUB_TYPE_MESSAGE_MESSAGE  'd'


typedef unsigned char* element_handle;
typedef unsigned char* stlv_packet;

//parse packet
int get_version(stlv_packet pack);
int get_body_length(stlv_packet pack);
int get_sequence(stlv_packet pack);
int get_flag(stlv_packet pack);

element_handle get_first_element(stlv_packet pack);
element_handle get_next_element(stlv_packet pack, element_handle handle);
element_handle get_first_sub_element(stlv_packet pack, element_handle parent);
element_handle get_next_sub_element(stlv_packet pack, element_handle parent, element_handle handle);

int get_element_type(stlv_packet pack, element_handle handle, char* buf, int buf_size);
int get_element_data_size(stlv_packet pack, element_handle handle, char* type_string, int str_len);
unsigned char* get_element_data_buffer(stlv_packet pack, element_handle handle, char* type_string, int str_len);

int filter_elements(
    stlv_packet pack,
    element_handle parent,
    element_handle begin,
    char* filter,
    int filter_size,
    element_handle* result
    );

//build packet
stlv_packet create_packet();
int send_packet(stlv_packet p, void (*callback)(int), int para);

int set_version    (stlv_packet p, int version);
int set_body_length(stlv_packet p, int len);
int set_sequence   (stlv_packet p, int sn);
int set_flag       (stlv_packet p, int flag);

element_handle append_element(stlv_packet p, element_handle parent /* = STLV_INVALID_HANDLE*/, char* type_buf, int buf_len);

#define ELEMENT_APPEND_TYPE(p, h, v) element_append_data(p, h, (uint8_t*)&v, sizeof(v))
int element_append_char  (stlv_packet p, element_handle h, char  data);
int element_append_short (stlv_packet p, element_handle h, short data);
int element_append_int   (stlv_packet p, element_handle h, int   data);
int element_append_data  (stlv_packet p, element_handle h, uint8_t* data_buf, int buf_len);
int element_append_string(stlv_packet p, element_handle h, char* data);

//utilities
#define STLV_BUF_BEGIN_TEMP_STRING(buf, size) { uint8_t temp = buf[size]
#define STLV_BUF_END_TEMP_STRING(buf, size) buf[size] = temp; }

#endif

