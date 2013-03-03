#include <stdio.h>

#include "sdp.h"
#include "avrcp.h"
#include "avctp.h"
#include "btstack/sdp_util.h"

/* Company IDs for vendor dependent commands */
#define IEEEID_BTSIG		0x001958

/* Error codes for metadata transfer */
#define E_INVALID_COMMAND	0x00
#define E_INVALID_PARAM		0x01
#define E_PARAM_NOT_FOUND	0x02
#define E_INTERNAL		0x03

/* Packet types */
#define AVRCP_PACKET_TYPE_SINGLE	0x00
#define AVRCP_PACKET_TYPE_START		0x01
#define AVRCP_PACKET_TYPE_CONTINUING	0x02
#define AVRCP_PACKET_TYPE_END		0x03

/* PDU types for metadata transfer */
#define AVRCP_GET_CAPABILITIES		0x10
#define AVRCP_LIST_PLAYER_ATTRIBUTES	0X11
#define AVRCP_LIST_PLAYER_VALUES	0x12
#define AVRCP_GET_CURRENT_PLAYER_VALUE	0x13
#define AVRCP_SET_PLAYER_VALUE		0x14
#define AVRCP_GET_PLAYER_ATTRIBUTE_TEXT	0x15
#define AVRCP_GET_PLAYER_VALUE_TEXT	0x16
#define AVRCP_DISPLAYABLE_CHARSET	0x17
#define AVRCP_CT_BATTERY_STATUS		0x18
#define AVRCP_GET_ELEMENT_ATTRIBUTES	0x20
#define AVRCP_GET_PLAY_STATUS		0x30
#define AVRCP_REGISTER_NOTIFICATION	0x31
#define AVRCP_REQUEST_CONTINUING	0x40
#define AVRCP_ABORT_CONTINUING		0x41
#define AVRCP_SET_ABSOLUTE_VOLUME	0x50

/* Capabilities for AVRCP_GET_CAPABILITIES pdu */
#define CAP_COMPANY_ID		0x02
#define CAP_EVENTS_SUPPORTED	0x03

#define AVRCP_REGISTER_NOTIFICATION_PARAM_LENGTH 5

#define AVRCP_FEATURE_CATEGORY_1	0x0001
#define AVRCP_FEATURE_CATEGORY_2	0x0002
#define AVRCP_FEATURE_CATEGORY_3	0x0004
#define AVRCP_FEATURE_CATEGORY_4	0x0008
#define AVRCP_FEATURE_PLAYER_SETTINGS	0x0010

enum battery_status {
	BATTERY_STATUS_NORMAL =		0,
	BATTERY_STATUS_WARNING =	1,
	BATTERY_STATUS_CRITICAL =	2,
	BATTERY_STATUS_EXTERNAL =	3,
	BATTERY_STATUS_FULL_CHARGE =	4,
};

static uint16_t   avrcp_service_buffer[50];

#pragma pack(1)
struct avrcp_header {
	uint8_t company_id[3];
	uint8_t pdu_id;
	uint8_t packet_type:2;
	uint8_t rsvd:6;
	uint16_t params_len;
	uint8_t params[0];
};
#define AVRCP_HEADER_LENGTH 7
#pragma pack()

static void handle_pdu(struct avrcp_header *pdu)
{

}

void avrcp_init()
{
  service_record_item_t * service_record_item;
  memset(avrcp_service_buffer, 0, sizeof(avrcp_service_buffer));
  service_record_item = (service_record_item_t *) avrcp_service_buffer;
  sdp_create_avrcp_service( (uint8_t*) &service_record_item->service_record, "AVRCP");
  printf("AVRCP service buffer size: %u\n", (uint16_t) (sizeof(service_record_item_t) + de_get_len((uint8_t*) &service_record_item->service_record)));
  //de_dump_data_element(service_record_item->service_record);
  sdp_register_service_internal(NULL, service_record_item);

  register_avctp_pid(0x110E, NULL);
}

/*
 * set_company_id:
 *
 * Set three-byte Company_ID into outgoing AVRCP message
 */
static void set_company_id(uint8_t cid[3], const uint32_t cid_in)
{
	cid[0] = cid_in >> 16;
	cid[1] = cid_in >> 8;
	cid[2] = cid_in;
}

int avrcp_set_volume(uint8_t volume)
{
  uint8_t buf[AVRCP_HEADER_LENGTH + 1];
  struct avrcp_header *pdu = (void *) buf;

  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_SET_ABSOLUTE_VOLUME;
  pdu->params[0] = volume;
  pdu->params_len = htons(1);

  return avctp_send_vendordep_resp(AVC_CTYPE_CONTROL, AVC_SUBUNIT_PANEL, buf, sizeof(buf));
}