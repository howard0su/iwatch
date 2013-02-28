
#include "l2cap.h"


#include "config.h"
#include "debug.h"

#include "avctp.h"

/* Message types */
#define AVCTP_COMMAND		0
#define AVCTP_RESPONSE		1

/* Packet types */
#define AVCTP_PACKET_SINGLE	0
#define AVCTP_PACKET_START	1
#define AVCTP_PACKET_CONTINUE	2
#define AVCTP_PACKET_END	3

static uint16_t l2cap_cid;
static void     *avctp_response_buffer;
static uint8_t  avctp_response_size;
static void     avctp_packet_handler(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size);

void avctp_init()
{
  l2cap_register_service_internal(NULL, avctp_packet_handler, AVCTP_PSM, 0xffff);
}

static void avctp_try_respond(void){
  if (!avctp_response_size ) return;
  if (!l2cap_cid) return;
  if (!l2cap_can_send_packet_now(l2cap_cid)) return;

  // update state before sending packet (avoid getting called when new l2cap credit gets emitted)
  uint16_t size = avctp_response_size;
  avctp_response_size = 0;
  l2cap_send_internal(l2cap_cid, avctp_response_buffer, size);
}

static void (*packet_handler) (uint8_t packet_type, uint8_t *packet, uint16_t size);

void register_avctp_pid(uint16_t pid, void (*handler)(uint8_t packet_type, uint8_t *packet, uint16_t size))
{
  packet_handler = handler;
}

static void avctp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  switch (packet_type) {
  case L2CAP_DATA_PACKET:
    {
      hexdump(packet, size);
      break;
    }
  case HCI_EVENT_PACKET:
    switch (packet[0]) {
    case L2CAP_EVENT_INCOMING_CONNECTION:
      {
        if (l2cap_cid)
        {
          l2cap_decline_connection_internal(channel, 0x0d);
        }
        else
        {
          l2cap_cid = channel;
          avctp_response_size = 0;
          l2cap_accept_connection_internal(channel);
        }
        break;
      }
    case L2CAP_EVENT_CREDITS:
    case DAEMON_EVENT_HCI_PACKET_SENT:
      avctp_try_respond();
      break;
    case L2CAP_EVENT_CHANNEL_CLOSED:
      break;
    }
  }
}