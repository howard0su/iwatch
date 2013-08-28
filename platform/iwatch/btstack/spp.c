#include "contiki.h"

#include "hci.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include "config.h"
#include "stlv.h"
#include "debug.h"

static service_record_item_t spp_service_record;
static const uint8_t   spp_service_buffer[100] = {
  0x36,0x00,0x61,0x09,0x00,0x00,0x0A,0x00,0x01,0x00,0x01,0x09,0x00,0x01,0x36,
  0x00,0x03,0x19,0x11,0x01,0x09,0x00,0x04,0x36,0x00,0x0E,0x36,0x00,0x03,0x19,
  0x01,0x00,0x36,0x00,0x05,0x19,0x00,0x03,0x08,0x01,0x09,0x00,0x05,0x36,0x00,
  0x03,0x19,0x10,0x02,0x09,0x00,0x06,0x36,0x00,0x09,0x09,0x65,0x6E,0x09,0x00,
  0x6A,0x09,0x01,0x00,0x09,0x00,0x09,0x36,0x00,0x09,0x36,0x00,0x06,0x19,0x11,
  0x01,0x09,0x01,0x00,0x09,0x01,0x00,0x25,0x10,0x69,0x57,0x61,0x74,0x63,0x68,
  0x20,0x43,0x6F,0x6E,0x66,0x69,0x67,0x75,0x72,0x65
};

#define SPP_CHANNEL 1
static uint16_t spp_channel_id = 0;

static char spp_buffer[1024];
static uint16_t spp_read_ptr = 0, spp_write_ptr = 0;


#define SPP_SENDER_NULL     0
#define SPP_SENDER_READY    1
#define SPP_SENDER_SENDING  2
#define SPP_SENDER_SENT     3

#define SPP_PACKET_SIZE  40

#define SPP_FLAG_BEGIN       0x01
#define SPP_FLAG_END         0x02
#define SPP_FLAG_BEGIN_END   (SPP_FLAG_BEGIN | SPP_FLAG_END)

typedef struct _spp_sender
{
    char* buffer;
    short buffer_size;
    short sent_size;
    short status;
    short unit_size;
    void (*callback)(int);
    int   para;
}spp_sender;

#define TASK_QUEUE_SIZE 10
static spp_sender task_queue[TASK_QUEUE_SIZE] = {0};
static short task_queue_pos = 0;

void tryToSend(void);
int spp_register_task(char* buf, int size, void (*callback)(int), int para)
{
    short cursor = task_queue_pos + 1;
    for (int i = 0; i < TASK_QUEUE_SIZE; ++i)
    {
        if (cursor >= TASK_QUEUE_SIZE)
            cursor = 0;

        spp_sender* task = &task_queue[cursor];
        if (task->status == SPP_SENDER_NULL)
        {
            task->buffer        = buf;
            task->buffer_size   = size;
            task->sent_size     = 0;
            task->unit_size     = 0;
            task->callback      = callback;
            task->para          = para;
            task->status        = SPP_SENDER_READY;
            tryToSend();
            return 0;
        }
        cursor++;
    }
    return -1;
}

static uint8_t build_transport_packet(spp_sender* task)
{
    short left_size = task->buffer_size - task->sent_size;
    short send_size = left_size > SPP_PACKET_SIZE ? SPP_PACKET_SIZE : left_size;

    unsigned char* flag_ptr = task->buffer + task->sent_size - 1;
    *flag_ptr = 0;
    if (send_size == left_size)
        *flag_ptr |= SPP_FLAG_END;
    if (task->sent_size == 0)
        *flag_ptr |= SPP_FLAG_BEGIN;

    return send_size;
}

static void tryToSend(void){
    if (!spp_channel_id) return;

    for (int i = 0; i < TASK_QUEUE_SIZE; ++i)
    {
        if (task_queue_pos >= TASK_QUEUE_SIZE)
            task_queue_pos = 0;

        spp_sender* task = &task_queue[task_queue_pos];

        if (task->status == SPP_SENDER_READY)
        {
            task->unit_size = build_transport_packet(task);
            task->status = SPP_SENDER_SENDING;
        }

        if (task->status == SPP_SENDER_SENDING)
        {
            int err = rfcomm_send_internal(spp_channel_id,
                task->buffer + task->sent_size - 1, task->unit_size + 1);
            if (err != 0)
                return;

            task->sent_size += task->unit_size;
            if (task->sent_size >= task->buffer_size)
            {
                if (task->callback != NULL)
                    task->callback(task->para);
                task->status = SPP_SENDER_NULL;
                task_queue_pos++;
            }
            else
            {
                task->status = SPP_SENDER_READY;
                task->unit_size = 0;
            }
            return;
        }

        task_queue_pos++;
    }
    return;

}


static unsigned char _recv_packet[STLV_PACKET_MAX_SIZE];
static short _recv_packet_size = 0;

static short handle_stvl_transport(unsigned char* packet, uint16_t size)
{
    if ((packet[0] & SPP_FLAG_BEGIN) != 0)
        _recv_packet_size = 0;

    if (_recv_packet_size + size - 1 > STLV_PACKET_MAX_SIZE)
    {
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

static void spp_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  uint16_t rfcomm_id;

  if (packet_type == RFCOMM_DATA_PACKET)
  {
    hexdump(packet, size);
    if (handle_stvl_transport(packet, size) > 0)
    {
        handle_stlv_packet(_recv_packet);
        _recv_packet_size = 0;
    }
    return;
  }

  if (packet_type == HCI_EVENT_PACKET)
  {
    switch(packet[0])
    {
    case RFCOMM_EVENT_INCOMING_CONNECTION:
      {
        uint8_t   rfcomm_channel_nr;
        bd_addr_t event_addr;
        // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
        bt_flip_addr(event_addr, &packet[2]);
        rfcomm_channel_nr = packet[8];
        rfcomm_id = READ_BT_16(packet, 9);
        log_info("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
        if (spp_channel_id == 0)
        {
          rfcomm_accept_connection_internal(rfcomm_id);
          break;
        }
        else
        {
          rfcomm_decline_connection_internal(rfcomm_id);
        }
        break;
      }
    case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
      {
        // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
        if (packet[2]) {
          log_info("RFCOMM channel open failed, status %u\n", packet[2]);
          spp_channel_id = 0;
        } else {
          spp_channel_id = READ_BT_16(packet, 12);
          uint16_t mtu = READ_BT_16(packet, 14);
          log_info("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_id, mtu);
          spp_write_ptr = spp_read_ptr = 0;
        }
        break;
      }
    case DAEMON_EVENT_HCI_PACKET_SENT:
    case RFCOMM_EVENT_CREDITS:
      {
        tryToSend();
        break;
      }
    case RFCOMM_EVENT_CHANNEL_CLOSED:
      {
        // data: event(8), len(8), rfcomm_cid(16)
        rfcomm_id = READ_BT_16(packet, 2);
        if (spp_channel_id == rfcomm_id)
        {
          spp_channel_id = 0;
        }
        break;
      }
    }
  }
}


static void sdp_create_spp_service(uint8_t *service, int service_id, const char *name){

        uint8_t* attribute;
        de_create_sequence(service);

    // 0x0000 "Service Record Handle"
        de_add_number(service, DE_UINT, DE_SIZE_16, SDP_ServiceRecordHandle);
        de_add_number(service, DE_UINT, DE_SIZE_32, 0x10001);

        // 0x0001 "Service Class ID List"
        de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_ServiceClassIDList);
        attribute = de_push_sequence(service);
        {
                de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1101 );
        }
        de_pop_sequence(service, attribute);

        // 0x0004 "Protocol Descriptor List"
        de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_ProtocolDescriptorList);
        attribute = de_push_sequence(service);
        {
                uint8_t* l2cpProtocol = de_push_sequence(attribute);
                {
                        de_add_number(l2cpProtocol,  DE_UUID, DE_SIZE_16, 0x0100);
                }
                de_pop_sequence(attribute, l2cpProtocol);

                uint8_t* rfcomm = de_push_sequence(attribute);
                {
                        de_add_number(rfcomm,  DE_UUID, DE_SIZE_16, 0x0003);  // rfcomm_service
                        de_add_number(rfcomm,  DE_UINT, DE_SIZE_8,  service_id);  // rfcomm channel
                }
                de_pop_sequence(attribute, rfcomm);
        }
        de_pop_sequence(service, attribute);

        // 0x0005 "Public Browse Group"
        de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_BrowseGroupList); // public browse group
        attribute = de_push_sequence(service);
        {
                de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1002 );
        }
        de_pop_sequence(service, attribute);

        // 0x0006
        de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_LanguageBaseAttributeIDList);
        attribute = de_push_sequence(service);
        {
                de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x656e);
                de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x006a);
                de_add_number(attribute, DE_UINT, DE_SIZE_16, 0x0100);
        }
        de_pop_sequence(service, attribute);

        // 0x0009 "Bluetooth Profile Descriptor List"
        de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_BluetoothProfileDescriptorList);
        attribute = de_push_sequence(service);
        {
                uint8_t *sppProfile = de_push_sequence(attribute);
                {
                        de_add_number(sppProfile,  DE_UUID, DE_SIZE_16, 0x1101);
                        de_add_number(sppProfile,  DE_UINT, DE_SIZE_16, 0x0100);
                }
                de_pop_sequence(attribute, sppProfile);
        }
        de_pop_sequence(service, attribute);

        // 0x0100 "ServiceName"
        de_add_number(service,  DE_UINT, DE_SIZE_16, 0x0100);
        de_add_data(service,  DE_STRING, strlen(name), (uint8_t *) name);
}


void spp_init()
{
  memset(&spp_service_record, 0, sizeof(spp_service_record));
  spp_service_record.service_record = (uint8_t*)&spp_service_buffer[0];
#if 0
  sdp_create_spp_service( spp_service_record.service_record, SPP_CHANNEL, "iWatch Configure");
  log_info("SDP service buffer size: %u\n", de_get_len(spp_service_record.service_record));
  hexdump((void*)spp_service_buffer, de_get_len(spp_service_record.service_record));
  //de_dump_data_element(service_record_item->service_record);
#endif
  sdp_register_service_internal(NULL, &spp_service_record);

  rfcomm_register_service_internal(NULL, spp_handler, SPP_CHANNEL, 100);  // reserved channel, mtu=100
}
