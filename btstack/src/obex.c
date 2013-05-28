#include "contiki.h"
#include "obex.h"

#include "hci.h"
#include "obex.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include "config.h"
#include "debug.h"


static uint16_t obex_response_size;
static void*    obex_response_buffer;
static uint16_t rfcomm_channel_id = 0;

static enum
{
  INITIALIZING,WAITINCOME,ERROR
}state;


static void obex_try_respond(uint16_t rfcomm_channel_id){
    if (!obex_response_size ) return;
    if (!rfcomm_channel_id) return;

    // update state before sending packet (avoid getting called when new l2cap credit gets emitted)
    uint16_t size = obex_response_size;
    obex_response_size = 0;
    if (rfcomm_send_internal(rfcomm_channel_id, obex_response_buffer, size) != 0)
    {
      // if error, we need retry
      obex_response_size = size;
    }
}

static void obex_handler(uint8_t type, uint16_t channelid, uint8_t *packet, uint16_t len)
{
  log_info("obex_handler state %d event %d\n", state, type);
  switch(type)
  {
  case RFCOMM_DATA_PACKET:
    {
      break;
    }
  case HCI_EVENT_PACKET:
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
          uint16_t rfcomm_id = READ_BT_16(packet, 9);
          log_info("RFCOMM channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
          if (rfcomm_channel_id == 0)
          {
            rfcomm_channel_id = rfcomm_id;
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
          if (packet[2])
          {
            rfcomm_channel_id = 0;
            break;
          }

          if (state == INITIALIZING)
          {
            state = WAITINCOME;
            obex_response_buffer = NULL;
            obex_response_size = 0;
            obex_try_respond(rfcomm_channel_id);
          }
          else
          {
            state = ERROR;
          }
          break;
        }
      case RFCOMM_EVENT_CREDITS:
        {
          obex_try_respond(rfcomm_channel_id);
          break;
        }
      case RFCOMM_EVENT_CHANNEL_CLOSED:
        {
          if (rfcomm_channel_id)
          {
            rfcomm_channel_id = 0;
          }
          state = INITIALIZING;
          break;
        }
      }
    }
  }
}

void obex_init(int channel)
{
  rfcomm_register_service_internal(NULL, obex_handler, channel, 100);  // reserved channel, mtu=100

  state = INITIALIZING;
}

void obex_open(const bd_addr_t *remote_addr, uint8_t port)
{
  if (rfcomm_channel_id)
    return;

  rfcomm_create_channel_internal(NULL, obex_handler, (bd_addr_t*)remote_addr, port);
}
