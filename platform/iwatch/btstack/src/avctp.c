
#include "l2cap.h"
#include "config.h"
#include "debug.h"

#include "avctp.h"

// bluetooth assgined number
#define AV_REMOTE_SVCLASS_ID		0x110e

/* Message types */
#define AVCTP_COMMAND		0
#define AVCTP_RESPONSE		1

/* Packet types */
#define AVCTP_PACKET_SINGLE	0
#define AVCTP_PACKET_START	1
#define AVCTP_PACKET_CONTINUE	2
#define AVCTP_PACKET_END	3

#pragma pack(1)
struct avctp_header {
	uint8_t ipid:1;
	uint8_t cr:1;
	uint8_t packet_type:2;
	uint8_t transaction:4;
	uint16_t pid;
};
#define AVCTP_HEADER_LENGTH 3

struct avc_header {
	uint8_t code:4;
	uint8_t _hdr0:4;
	uint8_t subunit_id:3;
	uint8_t subunit_type:5;
	uint8_t opcode;
};
#pragma pack()

static uint16_t l2cap_cid;
static void     *avctp_response_buffer;
static uint8_t  avctp_response_size;
static void     avctp_packet_handler(uint8_t packet_type, uint16_t channel,
                                     uint8_t *packet, uint16_t size);

#define MAX_PAYLOAD_SIZE 64
static uint8_t avctp_buf[AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH + MAX_PAYLOAD_SIZE];
static uint8_t id = 0;
static uint8_t need_send_release = 0;
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

  if (need_send_release)
  {
    struct avctp_header *avctp = (void *) avctp_buf;
    uint8_t *operands = &avctp_buf[AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH];

    need_send_release = 0;
    avctp_response_size = size;

    avctp->transaction = id++;
    operands[0] |= 0x80;
  }
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

int avctp_send_passthrough(uint8_t op)
{
  if (!l2cap_cid) return -1;

  struct avctp_header *avctp = (void *) avctp_buf;
  struct avc_header *avc = (void *) &avctp_buf[AVCTP_HEADER_LENGTH];
  uint8_t *operands = &avctp_buf[AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH];
  int sk;

  memset(avctp_buf, 0, AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH + 2);

  avctp->transaction = id++;
  avctp->packet_type = AVCTP_PACKET_SINGLE;
  avctp->cr = AVCTP_COMMAND;
  avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

  avc->code = AVC_CTYPE_CONTROL;
  avc->subunit_type = AVC_SUBUNIT_PANEL;
  avc->opcode = AVC_OP_PASSTHROUGH;

  operands[0] = op & 0x7f;
  operands[1] = 0;

  need_send_release = 1;

  avctp_response_size = AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH + 2;
  avctp_response_buffer = avctp_buf;

  return 0;
}

static int avctp_send(uint8_t transaction, uint8_t cr,
				uint8_t code, uint8_t subunit, uint8_t opcode,
				uint8_t *operands, size_t operand_count)
{
  uint8_t *buf;
  struct avctp_header *avctp;
  struct avc_header *avc;
  uint8_t *pdu;
  int sk, err = 0;
  uint16_t size;

  if (!l2cap_cid) return -1;
  if (operand_count > MAX_PAYLOAD_SIZE)
  {
    log_error("too long operand payload.\n");
    return -1;
  }

  size = AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH + operand_count;

  avctp = (void *) avctp_buf;
  avc = (void *) &avctp_buf[AVCTP_HEADER_LENGTH];
  pdu = (void *) &avctp_buf[AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH];

  avctp->transaction = transaction;
  avctp->packet_type = AVCTP_PACKET_SINGLE;
  avctp->cr = cr;
  avctp->pid = htons(AV_REMOTE_SVCLASS_ID);

  avc->code = code;
  avc->subunit_type = subunit;
  avc->opcode = opcode;

  memcpy(pdu, operands, operand_count);

  avctp_response_size = AVCTP_HEADER_LENGTH + AVC_HEADER_LENGTH + operand_count;
  avctp_response_buffer = avctp_buf;

  return 0;
}

int avctp_send_vendordep(uint8_t transaction,
				uint8_t code, uint8_t subunit,
				uint8_t *operands, size_t operand_count)
{
	return avctp_send(transaction, AVCTP_RESPONSE, code, subunit,
					AVC_OP_VENDORDEP, operands, operand_count);
}

