#include "contiki.h"

#include "hci.h"
#include "hfp.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include "config.h"
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

static void spp_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  uint16_t rfcomm_id;

  if (packet_type == RFCOMM_DATA_PACKET)
  {
    hexdump(packet, size);
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
          rfcomm_id = READ_BT_16(packet, 12);
          uint16_t mtu = READ_BT_16(packet, 14);
          log_info("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_id, mtu);
        }
        break;
      }
    case RFCOMM_EVENT_CREDITS:
      {
        // data: event(8), len(8), rfcomm_cid(16), credits(8)
        //rfcomm_id = READ_BT_16(packet, 2);
        //uint8_t credits = packet[4];
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