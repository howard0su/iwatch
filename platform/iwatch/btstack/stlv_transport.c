
#include "stlv_transport.h"

#include <stdio.h>
#include <string.h>
#include "stlv.h"

#ifdef UNITTEST
#   define send_internal test_send_internal
#   include "TestUtility/stlv_test_stub.h"
uint16_t spp_channel_id = 1;
#else
#   define send_internal rfcomm_send_internal
#   include "rfcomm.h"
extern uint16_t spp_channel_id;
#endif

extern void spp_sniff(int onoff);

typedef struct _spp_sender
{
    uint8_t* buffer;
    short    buffer_size;
    short    sent_size;
    short    status;
    short    unit_size;
    void    (*callback)(int);
    int     para;
}spp_sender;

#define TASK_QUEUE_SIZE 1
static spp_sender s_task = {0};
static uint8_t _task_queue_inited = 0;

int spp_register_task(uint8_t* buf, int size, void (*callback)(int), int para)
{
    if (!_task_queue_inited)
    {
        memset(&s_task, 0, sizeof(s_task));
        _task_queue_inited = 1;
    }

    spp_sender* task = &s_task;
    printf("spp_register_task(%d), task.status=%d\n", size, task->status);
    if (task->status == SPP_SENDER_NULL)
    {
        task->buffer      = buf;
        task->buffer_size = size;
        task->sent_size   = 0;
        task->unit_size   = 0;
        task->callback    = callback;
        task->para        = para;
        task->status      = SPP_SENDER_READY;
        if (size > 40)
        {
            printf("sniff(0)\n");
            spp_sniff(0);
        }
        tryToSend();
        return 0;
    }
    return -1;
}

static uint8_t build_transport_packet(spp_sender* task)
{
    short left_size = task->buffer_size - task->sent_size;
    short send_size = left_size > SPP_PACKET_SIZE ? SPP_PACKET_SIZE : left_size;

    uint8_t* flag_ptr = task->buffer + task->sent_size - 1;
    *flag_ptr = 0;
    if (send_size == left_size)
        *flag_ptr |= SPP_FLAG_END;
    if (task->sent_size == 0)
        *flag_ptr |= SPP_FLAG_BEGIN;

    return send_size;
}


void tryToSend(void)
{
    if (!spp_channel_id) return;

    spp_sender* task = &s_task;
    printf("tryToSend() enter_1 %d\n", task->status);
    //printf("try send STLV packet: status = %d\n", task->status);

    if (task->status == SPP_SENDER_READY)
    {
        task->unit_size = build_transport_packet(task);
        task->status = SPP_SENDER_SENDING;
    }
    printf("tryToSend() enter_2 %d\n", task->status);

    if (task->status == SPP_SENDER_SENDING)
    {
        int err = send_internal(spp_channel_id,
            task->buffer + task->sent_size - 1, task->unit_size + 1);
        //printf("send_internal(%d, %d) = %d\n", task->sent_size, task->unit_size, err);
        if (err != 0)
        {
            printf("send_internal(%d, %d) = %d ok\n", task->sent_size, task->unit_size, err);
            return;
        }

        task->sent_size += task->unit_size;
        printf("tryToSend(%d/%d)\n", task->sent_size, task->buffer_size);
        //printf("dump: %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x, ",
                //task->buffer[0], task->buffer[1], task->buffer[2], task->buffer[3],
                //task->buffer[4], task->buffer[5], task->buffer[6], task->buffer[7] );
        if (task->sent_size >= task->buffer_size)
        {
            task->status = SPP_SENDER_NULL;
            if (task->callback != NULL)
                task->callback(task->para);
        }
        else
        {
            
            task->status = SPP_SENDER_READY;
            task->unit_size = 0;
        }

    }

}

static uint8_t _recv_packet[STLV_PACKET_MAX_SIZE];
static short _recv_packet_size = 0;

void reset_stlv_transport_buffer()
{
    _recv_packet_size = 0;
}

uint8_t* get_stlv_transport_buffer()
{
    return _recv_packet;
}

short get_stlv_transport_buffer_size()
{
    return _recv_packet_size;
}

short handle_stvl_transport(unsigned char* packet, uint16_t size)
{
    printf("handle_stlv_transport:size = %d\n", size);
    if ((packet[0] & SPP_FLAG_BEGIN) != 0)
        _recv_packet_size = 0;

    if (_recv_packet_size + size - 1 > STLV_PACKET_MAX_SIZE)
    {

        printf("handle_stlv_transport error: packet too large(%d)\n", _recv_packet_size);
        _recv_packet_size = 0;
        return -1;
    }

    memcpy(&_recv_packet[_recv_packet_size], packet + 1, size - 1);
    _recv_packet_size += (size - 1);

    if ((packet[0] & SPP_FLAG_END) != 0)
        return _recv_packet_size;
    else
        return 0;
}


