#include <string.h>

#include "l2cap.h"
#include "config.h"
#include "debug.h"
#include "sdp.h"
#include <btstack/sdp_util.h>

static service_record_item_t mns_service_record;
static uint8_t   mns_service_buffer[100];

int mns_init(int channel)
{
  memset(&mns_service_record, 0, sizeof(mns_service_record));
  mns_service_record.service_record = (uint8_t*)&mns_service_buffer[0];
#if 1
  sdp_create_map_service( mns_service_record.service_record, channel, "MAP MNS-iWatch");
  log_info("MNS service buffer size: %u\n", de_get_len(mns_service_record.service_record));
  //hexdump((void*)spp_service_buffer, de_get_len(spp_service_record.service_record));
  de_dump_data_element(mns_service_record.service_record);
#endif
  sdp_register_service_internal(NULL, &mns_service_record);

  // register to obex

  return 0;
}

static uint16_t l2cap_cid;
static void sdpclient_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  switch (packet_type) {
  case L2CAP_DATA_PACKET:
    break;
  case HCI_EVENT_PACKET:
    {
      switch (packet[0]) {
      case L2CAP_EVENT_CHANNEL_OPENED:
        if (packet[2]) {
          // open failed -> reset
          l2cap_cid = 0;
        }
        else
        {
          l2cap_send_internal(l2cap_cid, NULL, 0);
        }
        break;
      }
    }
  }
}

int mns_open(bd_addr_t remote)
{
  // query the remote device to get the address

  l2cap_create_channel_internal(NULL, sdpclient_packet_handler, remote, PSM_SDP, 0xffff);

}
