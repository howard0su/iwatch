#include "stlv_client.h"

#include <stdio.h>
#include "stlv.h"

void send_echo(uint8_t* data, uint8_t size)
{
    stlv_packet p = create_packet();
    if (p == NULL)
        return;
    element_handle h = append_element(p, NULL, "E", 1);
    element_append_data(p, h, data, size);
    send_packet(p, 0, 0);
}

#define FILESENDER_S_NULL   0
#define FILESENDER_S_BEGIN  1
#define FILESENDER_S_DATA   2
#define FILESENDER_S_ENDING 3
#define FILESENDER_S_END    4

typedef struct _file_sender_t
{
    uint8_t  status;
    uint8_t  size;
    uint8_t* data;
    char*    name;
    int      para;
    void (*callback)(int);
}file_sender_t;

#define FILE_SENDER_COUNT 8
static uint8_t s_file_senders_status = 0;
static file_sender_t s_file_senders[FILE_SENDER_COUNT];

static void init_file_senders()
{
    for (uint8_t i = 0; i < FILE_SENDER_COUNT; ++i)
    {
        s_file_senders[i].status = FILESENDER_S_NULL;
    }
}

static void send_file_data_callback(int para)
{
    file_sender_t* s = &s_file_senders[para];
    if (s->status == FILESENDER_S_END)
    {
        s->status = FILESENDER_S_NULL;
    }
    else if (s->status == FILESENDER_S_ENDING)
    {
        printf("send end\n");
        //send the end flag packet
        stlv_packet p = create_packet();
        if (p == NULL)
            return;
        element_handle file_elem = append_element(p, NULL, "F", 1);
        element_handle endflag_elm = append_element(p, file_elem, "e", 1);
        element_append_char(p, endflag_elm, '\0');
        send_packet(p, send_file_data_callback, para);

        s->status = FILESENDER_S_END;
    }
    else if (s->callback != 0)
    {
        s->callback(s->para);
    }
}

int begin_send_file(char* name)
{
    if (s_file_senders_status == 0)
        init_file_senders();

    for (int i = 0; i < FILE_SENDER_COUNT; ++i)
    {
        if (s_file_senders[i].status == FILESENDER_S_NULL)
        {
            s_file_senders[i].status = FILESENDER_S_BEGIN;
            s_file_senders[i].name   = name;
            s_file_senders[i].size   = 0;
            s_file_senders[i].para   = 0;
            s_file_senders[i].callback = 0;
            s_file_senders[i].data   = 0;
            return i;
        }
    }
    return -1;
}

int send_file_data(int handle, uint8_t* data, uint8_t size, void (*callback)(int), int para)
{
    file_sender_t* s = &s_file_senders[handle];

    stlv_packet p = create_packet();
    if (p == NULL)
        return -1;
    element_handle file_elem = append_element(p, NULL, "F", 1);

    if (s->status == FILESENDER_S_BEGIN)
    {
        element_handle name_elm = append_element(p, file_elem, "n", 1);
        element_append_string(p, name_elm, s->name);
        s->status = FILESENDER_S_DATA;
    }

    element_handle data_elm = append_element(p, file_elem, "d", 1);
    element_append_data(p, data_elm, data, size);

    s->data = data;
    s->size = size;
    s->para = para;
    s->callback = callback;

    send_packet(p, send_file_data_callback, handle);
    return 0;
}

void end_send_file(int handle)
{
    file_sender_t* s = &s_file_senders[handle];
    s->status = FILESENDER_S_END;
}

