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
#define WAIT_CONNRESPONE 1
#define WAIT_REQUEST 2
#define WAIT_RESPONE 3
#define COMPBINE_PACKET 0x80

static void obex_handle_request(const struct obex* obex, const operation_obex* data)
{
  uint16_t length = data->length << 8 | data->length >> 8;
  const connection_obex *conn = (const connection_obex*)data;

  switch (data->opcode)
  {
      case 0x80:
      // connection request      
      obex->state_callback(OBEX_CB_CONNECT, conn->data, length - sizeof(connection_obex));
      break;
      case 0x81:
      // disconnect request
      obex->state_callback(OBEX_CB_DISCONNECT, conn->data, length - sizeof(connection_obex));
      break;
      case 0x02:
      obex->state_callback(OBEX_CB_PUT, data->data, length - sizeof(operation_obex));
      break;
      case 0x82:
      //put
      obex->state_callback(OBEX_CBFLAG_FINAL | OBEX_CB_PUT, 
        data->data, length - sizeof(operation_obex));
      break;
      case 0x03:
      obex->state_callback(OBEX_CB_GET, data->data, length - sizeof(operation_obex));
      break;
      case 0x83:
      obex->state_callback(OBEX_CBFLAG_FINAL | OBEX_CB_GET, data->data, 
        length - sizeof(operation_obex));
      //get
      break;
  }
}

static void obex_handle_response(const struct obex* obex, const operation_obex* data)
{

}

const uint8_t *obex_header_get_next(const uint8_t *prev, /* in,out*/ uint16_t *length_left)
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

  if (data->opcode != 0xA0)
  {
    printf("Connection error.\n");
    obex->state->state = INIT;
    return;
  }
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

  obex->state->state = WAIT_REQUEST;
  obex->state_callback(OBEX_CB_CONNECT_RESP, data->data, sizeof(uint32_t));
}

// handle the input packet and call event handler
void obex_handle(const struct obex* obex, const uint8_t* packet, uint16_t length)
{
  if ((obex->state->state & COMPBINE_PACKET) == COMPBINE_PACKET )
  {
    // append the packet
    uint16_t packet_length = READ_NET_16(obex->state->buffer, 1);
    memcpy(obex->state->buffer + obex->state->buffersize, packet, length);
    obex->state->buffersize += length;
    if (obex->state->buffersize >= packet_length)
    {
      packet = obex->state->buffer;
      length = obex->state->buffersize;
      obex->state->state &= ~COMPBINE_PACKET;
    }
    else
    {
      // need another packet
      return;
    }
  }
  else
  {
    // check if current packet is large enough
    uint16_t packet_length = READ_NET_16(packet, 1);
    if (packet_length > length)
    {
      memcpy(obex->state->buffer, packet, length);
      obex->state->buffersize = length;
      obex->state->state |= COMPBINE_PACKET;
      return;
    }
  }

  if (obex->state->state == WAIT_REQUEST || obex->state->state == INIT)
  {
    operation_obex *data = (operation_obex*)packet;
    obex_handle_request(obex, data);
    obex->state->state = WAIT_REQUEST;
    return;
  }

  if (obex->state->state == WAIT_RESPONE)
  {
    operation_obex *data = (operation_obex*)packet;
    obex_handle_response(obex, data);
  }
  else if (obex->state->state == WAIT_CONNRESPONE)
  {
    connection_obex *data = (connection_obex*)packet;    
    obex_handle_connected(obex, data);
  }

  if (*packet & 0x80)
  {
    obex->state->state = WAIT_REQUEST;
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

  return ptr;
}

uint8_t* obex_create_connect_request(const struct obex* obex, int opcode, uint8_t* buf)
{
  connection_obex *request = (connection_obex*)buf;
  request->opcode = opcode;
  request->length = 0;
  uint8_t *ptr = request->data;
  request->version = 0x10;
  request->flags = 0;
  net_store_16(buf, offsetof(connection_obex, max_packet_length), 255); // 255?

  return ptr;
}

void obex_send(const struct obex* obex, uint8_t* buf, uint16_t length)
{
  log_info("obex_send %d\n", length);
  net_store_16(buf, 1, length);
  obex->send(buf, length);  
}

void obex_connect_request(const struct obex* obex, const uint8_t *target, uint8_t target_length)
{
  uint8_t *ptr;
  uint8_t buf[128];

  if (obex->state->state != INIT)
    return;

  ptr = obex_create_connect_request(obex, 0x80, buf);

  if (target != NULL)
  {
    ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TARGET, target, target_length);
  }

  //putlenth
  net_store_16(buf, 1, ptr - buf);

  obex->state->state = WAIT_CONNRESPONE;
  obex->send(buf, ptr - buf);
}

void obex_init(const struct obex* obex)
{
  memset(obex->state, 0, sizeof(struct obex_state));
}