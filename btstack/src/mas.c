#include <string.h>

#include "l2cap.h"
#include "rfcomm.h"
#include "config.h"
#include "debug.h"
#include "sdp.h"
#include "obex.h"
#include <btstack/sdp_util.h>

/*
 * MAS implementation
 * Connect, EnableNotification
 */

static const uint8_t MAS_TARGET[16] =
{
 0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};

static void mas_callback(int code, uint8_t* lparam, uint16_t rparam);
static void mas_send(void *data, uint16_t length);
static uint16_t rfcomm_channel_id;
static struct obex_state mas_obex_state;
static const struct obex mas_obex = 
{
  &mas_obex_state, mas_callback, mas_send
};
static uint16_t mas_response_size;
static void*    mas_response_buffer;

static void mas_try_respond(uint16_t rfcomm_channel_id){
    if (!mas_response_size) return;
    if (!rfcomm_channel_id) return;

    // update state before sending packet (avoid getting called when new l2cap credit gets emitted)
    uint16_t size = mas_response_size;
    mas_response_size = 0;
    if (rfcomm_send_internal(rfcomm_channel_id, mas_response_buffer, size) != 0)
    {
      // if error, we need retry
      mas_response_size = size;
    }
    else
    {
      printf("MAS Sent %d byte: ", size);
      hexdump(mas_response_buffer, size);
    }
}

const static uint8_t appparams_notify[] = 
{
  0x0e,0x01,0x01
};
const static char type_notify[] = "x-bt/MAP-NotificationRegistration";

const static uint8_t appparams_getmessage[] = 
{
  0x0a, 0x01, 0x01, // Attachment
  0x14, 0x01, 0x01, //Charset
  0x15, 0x01, 0x00,//FractionRequest
};

const static char type_getmessage[] = "x-bt/message";

static uint8_t mas_buf[100];

static void mas_send(void *data, uint16_t length)
{
  mas_response_buffer = data;
  mas_response_size = length;

  mas_try_respond(rfcomm_channel_id);  
}

void mas_getmessage(char* id)
{
  printf("get message for %s\n", id);
  uint8_t *ptr = obex_create_request(&mas_obex, OBEX_OP_GET + OBEX_OP_FINAL, mas_buf);
  
  ptr = obex_header_add_uint32(ptr, OBEX_HEADER_CONNID, mas_obex.state->connection);
  ptr = obex_header_add_text(ptr, OBEX_HEADER_NAME, id);
  ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TYPE, (uint8_t*)type_getmessage, sizeof(type_getmessage));
  ptr = obex_header_add_bytes(ptr, OBEX_HEADER_APPPARMS, appparams_getmessage, sizeof(appparams_getmessage));
    
  obex_send(&mas_obex, mas_buf, ptr - mas_buf);  
}

static void mas_callback(int code, uint8_t* lparam, uint16_t rparam)
{
  printf("Callback with code %d\n", code);
  switch(code)
  {
    case OBEX_CB_CONNECT_RESP: // connected
    {
      uint8_t *ptr = obex_create_request(&mas_obex, OBEX_OP_PUT + OBEX_OP_FINAL, mas_buf);
      uint8_t Fillerbyte = 0x30;
      
      ptr = obex_header_add_uint32(ptr, OBEX_HEADER_CONNID, mas_obex.state->connection);
      ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TYPE, (uint8_t*)type_notify, sizeof(type_notify));
      ptr = obex_header_add_bytes(ptr, OBEX_HEADER_APPPARMS, appparams_notify, sizeof(appparams_notify));
      ptr = obex_header_add_bytes(ptr, OBEX_HEADER_ENDBODY, &Fillerbyte, 1);
        
      obex_send(&mas_obex, mas_buf, ptr - mas_buf);
      break;
    }
  }
}

static void mas_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  switch(packet_type)
  {
  case RFCOMM_DATA_PACKET:
    printf("MAS received: ");
    hexdump(packet, size);
    obex_handle(&mas_obex, packet, size);
    rfcomm_grant_credits(rfcomm_channel_id, 1); // get the next packet
    break;
  case DAEMON_EVENT_PACKET:
    switch(packet[0])    
    {
      case DAEMON_EVENT_NEW_RFCOMM_CREDITS:
      case DAEMON_EVENT_HCI_PACKET_SENT:
      case RFCOMM_EVENT_CREDITS:
        {
          mas_try_respond(rfcomm_channel_id);
          break;
        }
    }
    break;
  case HCI_EVENT_PACKET:
    {
      switch(packet[0])
      {
      case RFCOMM_EVENT_INCOMING_CONNECTION:
        break;
      case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
        {
          if (packet[2])
          {
            rfcomm_channel_id = 0;
          }
          else
          {
            rfcomm_channel_id = READ_BT_16(packet, 12);
            printf("MAS connection is created channel = %d.\n", rfcomm_channel_id);
            obex_connect_request(&mas_obex, MAS_TARGET, sizeof(MAS_TARGET));
          }
          break;
        }
      case DAEMON_EVENT_NEW_RFCOMM_CREDITS:
      case DAEMON_EVENT_HCI_PACKET_SENT:
      case RFCOMM_EVENT_CREDITS:
        {
          mas_try_respond(rfcomm_channel_id);
          break;
        }
      case RFCOMM_EVENT_CHANNEL_CLOSED:
        {
          if (rfcomm_channel_id)
          {
            rfcomm_channel_id = 0;
          }
          break;
        }
      }
    }
  }
}


int mas_open(const bd_addr_t *remote_addr, uint8_t port)
{
  mas_response_size = 0;
  obex_init(&mas_obex);
  rfcomm_create_channel_internal(NULL, mas_packet_handler, (bd_addr_t*)remote_addr, port);

  return 0;
}