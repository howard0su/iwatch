#include <string.h>

#include "l2cap.h"
#include "rfcomm.h"
#include "config.h"
#include "debug.h"
#include "sdp.h"
#include "obex.h"
#include <btstack/sdp_util.h>
#include <btstack/utils.h>

#define MNS_CHANNEL 17

static void mns_callback(int code, const uint8_t* lparam, uint16_t rparam);
static void mns_send(void *data, uint16_t length);

static service_record_item_t mns_service_record;
static uint8_t  mns_service_buffer[100];
static uint16_t rfcomm_channel_id;
static struct obex_state mns_obex_state;
static const struct obex mns_obex = 
{
  &mns_obex_state, mns_callback, mns_send
};
static uint16_t mns_response_size;
static void*    mns_response_buffer;

enum {STATE_0} state;

static const uint8_t MNS_TARGET[16] =
{
  0xbb, 0x58, 0x2b, 0x41, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};


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

static void mns_try_respond(uint16_t rfcomm_channel_id){
    if (!mns_response_size) return;
    if (!rfcomm_channel_id) return;

    // update state before sending packet (avoid getting called when new l2cap credit gets emitted)
    uint16_t size = mns_response_size;
    mns_response_size = 0;
    if (rfcomm_send_internal(rfcomm_channel_id, mns_response_buffer, size) != 0)
    {
      // if error, we need retry
      mns_response_size = size;
    }
    else
    {
      hexdump(mns_response_buffer, size);
    }
}

static void mns_send(void *data, uint16_t length)
{
  mns_response_buffer = data;
  mns_response_size = length;

  mns_try_respond(rfcomm_channel_id);  
}

static void mns_callback(int code, const uint8_t* header, uint16_t length)
{
  printf("Callback with code %d\n", code);
  uint8_t buf[20];

  switch(code)
  {
    case OBEX_CB_CONNECT:
    // client try to make a connection
    // validate the target
    while(header != NULL && length > 0)
    {
      switch(*header)
      {
        case OBEX_HEADER_TARGET:
        {
          if (READ_NET_16(header, 1) == (16 + 3) &&
            memcmp((uint8_t*)header + 3, MNS_TARGET, 16) == 0)
          {
            // send response
              printf("connection successful\n");
              uint8_t *ptr = obex_create_connect_request(&mns_obex, 0xA0, buf);
              ptr = obex_header_add_uint32(ptr, OBEX_HEADER_CONNID, 0x123456);
              obex_send(&mns_obex, buf, ptr - buf);
          }
          else
          {
              printf("connection failed\n");
              uint8_t *ptr = obex_create_connect_request(&mns_obex, 404, buf);
              obex_send(&mns_obex, buf, ptr - buf);
          }
          return;
        }
      }

      header = obex_header_get_next(header, &length);
    }

    obex_create_request(&mns_obex, 500, buf);
    obex_send(&mns_obex, buf, 0);

    break;
  case OBEX_CB_PUT:
    while(header != NULL && length > 0)
    {
      switch(*header)
      {
        case OBEX_HEADER_BODY:
        {
          uint16_t length = READ_NET_16((uint8_t*)header, 1);
          hexdump((uint8_t*)header + 3, length);
          obex_create_request(&mns_obex, 0x20, buf);
          obex_send(&mns_obex, buf, 0);
          break;
        }
      }

      header = obex_header_get_next(header, &length);
    }
    break;
  case OBEX_CB_PUT | OBEX_CBFLAG_FINAL:
    while(header != NULL && length > 0)
    {
      switch(*header)
      {
        case OBEX_HEADER_BODY:
        case OBEX_HEADER_ENDBODY:
        {
          uint16_t length = READ_NET_16((uint8_t*)header, 1);
          hexdump((uint8_t*)header + 3, length);
          obex_create_request(&mns_obex, 0xA0, buf);
          obex_send(&mns_obex, buf, 0);
          break;
        }
      }
      header = obex_header_get_next(header, &length);
    }
    break;
  }
}

static void mns_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  switch(packet_type)
  {
  case RFCOMM_DATA_PACKET:
    hexdump(packet, size);
    obex_handle(&mns_obex, packet, size);
    break;
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
          log_info("MNS channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
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
          }
          else
          {
            obex_init(&mns_obex);
          }
          break;
        }
      case RFCOMM_EVENT_CREDITS:
        {
          mns_try_respond(rfcomm_channel_id);
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

int mns_init()
{
  memset(&mns_service_record, 0, sizeof(mns_service_record));
  mns_service_record.service_record = (uint8_t*)&mns_service_buffer[0];
#if 1
  sdp_create_map_service( (uint8_t*)&mns_service_buffer[0], MNS_CHANNEL, "MAP MNS-Kreyos");
  log_info("MNS service buffer size: %u\n", de_get_len(mns_service_record.service_record));
  //hexdump((void*)mns_service_buffer, de_get_len(mns_service_record.service_record));
  //de_dump_data_element(mns_service_record.service_record);
#endif
  sdp_register_service_internal(NULL, &mns_service_record);

  // register to obex
  rfcomm_register_service_internal(NULL, mns_packet_handler, MNS_CHANNEL, 0xffff);
  return 0;
}



