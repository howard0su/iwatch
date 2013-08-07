#include "contiki.h"
#include "obex.h"

#include "hci.h"
#include "obex.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/utils.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include <assert.h>
#include "config.h"
#include "debug.h"

#define INIT 0
#define CONNECTING 1
#define CONNECTED 2
#define WAITRESP 3


static void obex_handle_request(const struct obex* obex, const operation_obex* data)
{

}

static void obex_handle_response(const struct obex* obex, const operation_obex* data)
{

}

static void obex_handle_connected(const struct obex* obex, const connection_obex* data)
{
  const uint8_t *ptr = data->data;
  while(ptr - data->data < data->length)
  {
    switch(*ptr)
    {
      case OBEX_HEADER_CONNID:
        obex->state->connection = READ_NET_32(ptr, 0);
        ptr += 4 + 1;
        break;
    }
  }

  obex->callback(OBEX_CB_NEWCONN, &(obex->state->connection), sizeof(uint32_t));
  obex->state->state = CONNECTED;
}

// handle the input packet and call event handler
void obex_handle(const struct obex* obex, const uint8_t* packet, uint16_t length)
{
  switch(*packet)
  {
    case 0x20:
    case 0xA0:
    {
      // if sucess, get connectid
      switch (obex->state->state)
      {
        case CONNECTING:
        {
          connection_obex *data = (connection_obex*)packet;    
          obex_handle_connected(obex, data);
          break;
        }
        case CONNECTED:
        {
          operation_obex *data = (operation_obex*)packet;
          obex_handle_request(obex, data);
          break;
        }
        case WAITRESP:
        {
          operation_obex *data = (operation_obex*)packet;
          obex_handle_response(obex, data);
          break;
        }
      }
      break;
    }
    default:
      printf("Response error: %d\n", *packet);
      break;
  }
}

uint8_t* obex_header_add_text(uint8_t *buf, int code, const char* text)
{
  // assert code
  assert((code & 0xC0) == 0x00);
  buf[0] = code;
  int length = strlen(text);
  net_store_16(buf, 1, length);
  memcpy(buf + 3, text, length + 1);

  return buf + 3 + length + 1;
}

uint8_t* obex_header_add_bytes(uint8_t *buf, int code, const uint8_t *data, int length)
{
  // assert code
  assert((code & 0xC0) == 0x40);
  buf[0] = code;
  net_store_16(buf, 1, length);
  memcpy(buf + 3, data, length);

  return buf + 3 + length;
}

uint8_t *obex_header_add_byte(uint8_t *buf, int code, uint8_t data)
{
  assert((code & 0xC0) == 0x80);
  buf[0] = code;
  buf[1] = data;
  return buf + 2;
}

uint8_t *obex_header_add_uint32(uint8_t *buf, int code, uint32_t data)
{
  assert((code & 0xC0) == 0xC0);
  buf[0] = code;
  net_store_32(buf, 1, data);

  return buf + 5;
}

uint8_t* obex_create_request(const struct obex* obex, int opcode, uint8_t* buf)
{
  operation_obex *request = (operation_obex*)buf;
  request->opcode = opcode;
  uint8_t *ptr = request->data;
  obex_header_add_uint32(ptr, OBEX_HEADER_CONNID, obex->state->connection);

  return ptr;
}

void obex_send(const struct obex* obex, uint8_t* buf, uint16_t length)
{
  if (obex->state->state != CONNECTED)
    return;
  operation_obex *request = (operation_obex*)buf;

  net_store_16(buf, 1, length);

  obex->state->state = WAITRESP;
  obex->callback(OBEX_CB_SEND, buf, length);  
}

void obex_connect(const struct obex* obex, const uint8_t *target, uint8_t target_length)
{
  uint8_t *ptr;
  uint8_t buf[128];

  if (obex->state->state != INIT)
    return;

  connection_obex* conn = (connection_obex*)buf;
  conn->opcode = 0x80; // connect
  conn->version = 0x10;
  conn->flags = 0;
  conn->max_packet_length = 0x2800; // 255?

  ptr = conn->data;
  if (target != NULL)
  {
    ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TARGET, target, target_length);
  }

  //putlenth
  net_store_16(buf, 1, ptr - buf);

  obex->state->state = CONNECTING;
  obex->callback(OBEX_CB_SEND, buf, ptr - buf);
}

void obex_init(const struct obex* obex)
{
  memset(obex->state, 0, sizeof(struct obex_state));
}