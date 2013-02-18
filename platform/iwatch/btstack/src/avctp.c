
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

static void avctp_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

void avctp_init()
{
  l2cap_register_service_internal(NULL, avctp_packet_handler, AVCTP_PSM, 0xffff);
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
    break;
    }
  case HCI_EVENT_PACKET:
    switch (packet[0]) {
    case L2CAP_EVENT_INCOMING_CONNECTION:
      {
        break;
      }
    case L2CAP_EVENT_CREDITS:
    case DAEMON_EVENT_HCI_PACKET_SENT:
      break;
    case L2CAP_EVENT_CHANNEL_CLOSED:
      break;
    }
  }
}