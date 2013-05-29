#include <string.h>

#include "l2cap.h"
#include "rfcomm.h"
#include "config.h"
#include "debug.h"
#include "sdp.h"
#include <btstack/sdp_util.h>

#define MNS_CHANNEL 17

static service_record_item_t mns_service_record;
static uint8_t   mns_service_buffer[100];

enum {STATE_0} state;


static void sdp_create_map_service(uint8_t *service, int service_id, const char *name) {

	uint8_t* attribute;
	de_create_sequence(service);

        // 0x0000 "Service Record Handle"
	de_add_number(service, DE_UINT, DE_SIZE_16, SDP_ServiceRecordHandle);
	de_add_number(service, DE_UINT, DE_SIZE_32, 0x10004);

	// 0x0001 "Service Class ID List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_ServiceClassIDList);
	attribute = de_push_sequence(service);
	{
		de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1133 );
	}
	de_pop_sequence(service, attribute);

	// 0x0004 "Protocol Descriptor List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_ProtocolDescriptorList);
	attribute = de_push_sequence(service);
	{
		uint8_t* l2cpProtocol = de_push_sequence(attribute);
		{
			de_add_number(l2cpProtocol,  DE_UUID, DE_SIZE_16, 0x0100);
		}
		de_pop_sequence(attribute, l2cpProtocol);

		uint8_t* rfcomm = de_push_sequence(attribute);
		{
			de_add_number(rfcomm,  DE_UUID, DE_SIZE_16, 0x0003);  // rfcomm_service
			de_add_number(rfcomm,  DE_UINT, DE_SIZE_8,  service_id);  // rfcomm channel
		}
		de_pop_sequence(attribute, rfcomm);

                uint8_t* obex = de_push_sequence(attribute);
		{
			de_add_number(obex,  DE_UUID, DE_SIZE_16, 0x0008);
		}
		de_pop_sequence(attribute, obex);
	}
	de_pop_sequence(service, attribute);

	// 0x0009 "Bluetooth Profile Descriptor List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_BluetoothProfileDescriptorList);
	attribute = de_push_sequence(service);
	{
		uint8_t *mapProfile = de_push_sequence(attribute);
		{
			de_add_number(mapProfile,  DE_UUID, DE_SIZE_16, 0x1134);
                        de_add_number(mapProfile,  DE_UINT, DE_SIZE_16, 0x0100);
		}
		de_pop_sequence(attribute, mapProfile);
	}
	de_pop_sequence(service, attribute);

	// 0x0100 "ServiceName"
	de_add_number(service,  DE_UINT, DE_SIZE_16, 0x0100);
	de_add_data(service,  DE_STRING, strlen(name), (uint8_t *) name);
}

static void mns_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

}

int mns_init()
{
  memset(&mns_service_record, 0, sizeof(mns_service_record));
  mns_service_record.service_record = (uint8_t*)&mns_service_buffer[0];
#if 1
  sdp_create_map_service( (uint8_t*)&mns_service_buffer[0], MNS_CHANNEL, "MAP MNS-iWatch");
  log_info("MNS service buffer size: %u\n", de_get_len(mns_service_record.service_record));
  //hexdump((void*)mns_service_buffer, de_get_len(mns_service_record.service_record));
  //de_dump_data_element(mns_service_record.service_record);
#endif
  sdp_register_service_internal(NULL, &mns_service_record);

  // register to obex
  rfcomm_register_service_internal(NULL, mns_packet_handler, MNS_CHANNEL, 0xffff);
  return 0;
}

static uint16_t rfcomm_channel_id;
static void mas_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{

  switch(packet_type)
  {
  case RFCOMM_DATA_PACKET:
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
            break;
          }
          else
          {
            rfcomm_channel_id = READ_BT_16(packet, 9);
          }
        }
      case RFCOMM_EVENT_CREDITS:
        {
          //hfp_try_respond(rfcomm_channel_id);
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


int mns_open(const bd_addr_t *remote_addr, uint8_t port)
{
  rfcomm_create_channel_internal(NULL, mas_packet_handler, (bd_addr_t*)remote_addr, port);

  return 0;
}