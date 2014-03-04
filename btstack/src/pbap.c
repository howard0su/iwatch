#include "obex.h"

static struct obex_state pbap_obex_state;

static const struct obex pbap_obex = 
{
  &pbap_obex_state, pbap_callback, pbap_send
};

static void pbap_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  switch(packet_type)
  {
  case RFCOMM_DATA_PACKET:
//    printf("pbap received: ");
//    hexdump(packet, size);
    obex_handle(&pbap_obex, packet, size);
    rfcomm_grant_credits(rfcomm_channel_id, 1); // get the next packet
    break;
  case DAEMON_EVENT_PACKET:
    switch(packet[0])    
    {
      case DAEMON_EVENT_NEW_RFCOMM_CREDITS:
      case DAEMON_EVENT_HCI_PACKET_SENT:
      case RFCOMM_EVENT_CREDITS:
        {
          pbap_try_respond(rfcomm_channel_id);
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
            printf("pbap connection is created channel = %d.\n", rfcomm_channel_id);
            obex_connect_request(&pbap_obex, pbap_TARGET, sizeof(pbap_TARGET));
          }
          break;
        }
      case DAEMON_EVENT_NEW_RFCOMM_CREDITS:
      case DAEMON_EVENT_HCI_PACKET_SENT:
      case RFCOMM_EVENT_CREDITS:
        {
          pbap_try_respond(rfcomm_channel_id);
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


int pbap_open(const bd_addr_t *remote_addr, uint8_t port)
{
  pbap_response_size = 0;
  obex_init(&pbap_obex);
  rfcomm_create_channel_internal(NULL, pbap_packet_handler, (bd_addr_t*)remote_addr, port);

  return 0;
}