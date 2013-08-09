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
#include <stddef.h>
#include "config.h"
#include "debug.h"

#define INIT 0
#define CONNECTING 1
#define CONNECTED 2
#define WAITRESP 3


static void obex_handle_request(const struct obex* obex, const operation_obex* data)
{
  switch (data->opcode)
  {
      case 0x80:
      // connection request      
      break;
      case 0x81:
      // disconnect request
      break;
      case 0x02:
      case 0x82:
      //put
      break;
      case 0x03:
      case 0x83:
      //get
      break;
  }
}

static void obex_handle_response(const struct obex* obex, const operation_obex* data)
{

}

uint8_t *obex_header_get_next(uint8_t *prev, /* in,out*/ uint16_t *length_left)
{
  if (*length_left == 0)
    return NULL;
  uint16_t prev_lenth = 0;
  switch (*prev & 0xC0)
  {
    case 0x00:
    case 0x40:
      // skip 
      prev_lenth = READ_NET_16(prev, 1);
      break;
    case 0x80:
      prev_lenth = 2;
      break;
    case 0xC0:
      prev_lenth = 5;
      break;
    default:
      return NULL;
  }

  if (prev_lenth <= *length_left)
  {
    *length_left -= prev_lenth;
    return prev + prev_lenth;
  }

  *length_left = 0;
  return NULL;
}

static void obex_handle_connected(const struct obex* obex, const connection_obex* data)
{
  const uint8_t *ptr = data->data;
  uint16_t length = data->length << 8 | data->length >> 8;
  length -= sizeof(connection_obex);

  while(ptr != NULL && length > 0)
  {
    switch(*ptr)
    {
      case OBEX_HEADER_CONNID:
        obex->state->connection = READ_NET_32(ptr, 1);
        break;
    }

    ptr = obex_header_get_next(ptr, &length);
  }

  obex->state->state = CONNECTED;
  obex->state_callback(OBEX_CB_NEWCONN, &(obex->state->connection), sizeof(uint32_t));
}

// handle the input packet and call event handler
void obex_handle(const struct obex* obex, const uint8_t* packet, uint16_t length)
{
  if (obex->state->state == CONNECTED)
  {
    operation_obex *data = (operation_obex*)packet;
    obex_handle_request(obex, data);
    return;
  }

  // response
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

uint8_t* obex_header_add_text(uint8_t *buf, int code, const wchar_t* text)
{
  // assert code
  assert((code & 0xC0) == 0x00);
  buf[0] = code;
  int length = wcslen(text) + 1;
  net_store_16(buf, 1, length * 2 + 3);
  memcpy(buf + 3, text, length * 2);

  return buf + 3 + length * 2;
}

uint8_t* obex_header_add_bytes(uint8_t *buf, int code, const uint8_t *data, int length)
{
  // assert code
  assert((code & 0xC0) == 0x40);
  buf[0] = code;
  net_store_16(buf, 1, length + 3);
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
  request->length = 0;
  uint8_t *ptr = request->data;
  ptr = obex_header_add_uint32(ptr, OBEX_HEADER_CONNID, obex->state->connection);

  return ptr;
}

void obex_send(const struct obex* obex, uint8_t* buf, uint16_t length)
{
  log_info("obex_send %d\n", length);
  if (obex->state->state != CONNECTED)
    return;

  net_store_16(buf, 1, length);

  obex->state->state = WAITRESP;
  obex->send(buf, length);  
}

void obex_connect_request(const struct obex* obex, const uint8_t *target, uint8_t target_length)
{
  uint8_t *ptr;
  uint8_t buf[128];

  if (obex->state->state != INIT)
    return;

  connection_obex* conn = (connection_obex*)buf;
  conn->opcode = 0x80; // connect
  conn->version = 0x10;
  conn->flags = 0;
  net_store_16(buf, offsetof(connection_obex, max_packet_length), 42); // 255?

  ptr = conn->data;
  if (target != NULL)
  {
    ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TARGET, target, target_length);
  }

  //putlenth
  net_store_16(buf, 1, ptr - buf);

  obex->state->state = CONNECTING;
  obex->send(buf, ptr - buf);
}



void obex_init(const struct obex* obex)
{
  memset(obex->state, 0, sizeof(struct obex_state));
}