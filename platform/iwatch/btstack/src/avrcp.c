#include <stdio.h>
#include <string.h>

#include "l2cap.h"
#include "config.h"
#include "debug.h"

#include "sdp.h"
#include "avrcp.h"
#include "avctp.h"
#include "btstack/sdp_util.h"
#include "btstack/utils.h"

#define htons(x) __swap_bytes(x)

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

static service_record_item_t avrcp_service_record;
static const uint8_t   avrcp_service_buffer[87] =
{
  0x36,0x00,0x54,0x09,0x00,0x00,0x0A,0x00,0x01,0x00,0x03,0x09,0x00,0x01,0x36,
  0x00,0x06,0x19,0x11,0x0E,0x19,0x11,0x0F,0x09,0x00,0x04,0x36,0x00,0x12,0x36,
  0x00,0x06,0x19,0x01,0x00,0x09,0x00,0x17,0x36,0x00,0x06,0x19,0x00,0x17,0x09,
  0x01,0x04,0x09,0x00,0x05,0x36,0x00,0x03,0x19,0x10,0x02,0x09,0x00,0x09,0x36,
  0x00,0x09,0x36,0x00,0x06,0x19,0x11,0x0E,0x09,0x01,0x05,0x09,0x01,0x00,0x25,
  0x05,0x41,0x56,0x52,0x43,0x50,0x09,0x03,0x11,0x09,0x00,0x01
};

// callback to UI system
static windowproc callback_handler;

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

int avrcp_get_attributes(uint32_t id);

static void handle_notification(struct avrcp_header *pdu )
{
  switch(pdu->params[0])
  {
  case AVRCP_EVENT_STATUS_CHANGED:
    log_info("current status is %d\n", pdu->params[1]);
    if (callback_handler)
    {
      callback_handler(AVRCP_EVENT_STATUS_CHANGED, pdu->params[1], NULL);
    }
    break;
  case AVRCP_EVENT_TRACK_CHANGED:
    avrcp_get_attributes(0);
    break;
  }
}

static void handle_attributes(struct avrcp_header *pdu)
{
  uint8_t elementnum = pdu->params[0];
  uint16_t offset = 1;
  for (int i = 0; i < elementnum; i++)
  {
    uint32_t attributeid = READ_NET_32(pdu->params, offset);
    offset += 4;
    uint16_t charsetid = READ_NET_16(pdu->params, offset);
    offset += 2;
    uint16_t len = READ_NET_16(pdu->params, offset);
    offset += 2;
    if (len > 0)
    {
      pdu->params[offset + len] = 0;
      printf("attribute %ld charset %d len %d : %s\n", attributeid, charsetid,
              len, &pdu->params[offset]);
      if (callback_handler)
      {
        callback_handler(AVRCP_EVENT_TRACK_CHANGED, attributeid, &pdu->params[offset]);
      }
      offset += len;
    }
  }
}

static void handle_playstatus(struct avrcp_header* pdu)
{
  uint32_t length = READ_NET_32(pdu->params, 0);
  uint32_t pos = READ_NET_32(pdu->params, 4);

  uint8_t status = pdu->params[8];

  log_info("play status : %ld of %ld, status: %d\n", pos, length, (uint16_t)status);

  return;
}

static void handle_pdu(uint8_t *data, uint16_t size)
{
  struct avrcp_header *pdu = (struct avrcp_header *)data;


  if (data == NULL)
  {
    // special event
    switch(size)
    {
    case 0:
      callback_handler(AVRCP_EVENT_DISCONNECTED, 0, NULL);
      // disconect
      break;
    case 1:
      // connected
      callback_handler(AVRCP_EVENT_CONNECTED, 0, NULL);
      break;
    }
  }


  switch(pdu->pdu_id)
  {
  case AVRCP_REGISTER_NOTIFICATION:
    handle_notification(pdu);
    break;
  case AVRCP_GET_CAPABILITIES:

    break;
  case AVRCP_GET_ELEMENT_ATTRIBUTES:
    handle_attributes(pdu);
    break;
  case AVRCP_GET_PLAY_STATUS:
    handle_playstatus(pdu);
    break;
  }
}

int avrcp_register_handler(windowproc proc)
{
  callback_handler = proc;
  return 0;
}

void avrcp_init()
{
  memset(&avrcp_service_record, 0, sizeof(avrcp_service_record));
  avrcp_service_record.service_record = (uint8_t*)&avrcp_service_buffer[0];
#if 0
  sdp_create_avrcp_service(avrcp_service_record.service_record, "AVRCP");
  log_info("SDP service buffer size: %u\n", de_get_len(avrcp_service_record.service_record));
  hexdump((void*)avrcp_service_buffer, de_get_len(avrcp_service_record.service_record));
  //de_dump_data_element(service_record_item->service_record);
#endif
  sdp_register_service_internal(NULL, &avrcp_service_record);

  avctp_register_pid(0x110E, handle_pdu);
}

/*
* set_company_id:
*
* Set three-byte Company_ID into outgoing AVRCP message
*/
static inline void set_company_id(uint8_t cid[3], const uint32_t cid_in)
{
  cid[0] = cid_in >> 16;
  cid[1] = cid_in >> 8;
  cid[2] = cid_in;
}


int avrcp_enable_notification(uint8_t id)
{
  uint8_t buf[AVRCP_HEADER_LENGTH + 5];
  struct avrcp_header *pdu = (void *) buf;

  memset(buf, 0, sizeof(buf));

  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_REGISTER_NOTIFICATION;
  //pdu->packet_type = AVRCP_PACKET_TYPE_SINGLE;
  pdu->params[0] = id;
  pdu->params_len = htons(5);

  return avctp_send_vendordep(AVC_CTYPE_NOTIFY, AVC_SUBUNIT_PANEL,
                              buf, 5 + AVRCP_HEADER_LENGTH);
}

int avrcp_set_volume(uint8_t volume)
{
  uint8_t buf[AVRCP_HEADER_LENGTH + 1];
  struct avrcp_header *pdu = (void *) buf;

  memset(buf, 0, sizeof(buf));
  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_SET_ABSOLUTE_VOLUME;
  pdu->params[0] = volume;
  pdu->params_len = htons(1);

  return avctp_send_vendordep(AVC_CTYPE_CONTROL, AVC_SUBUNIT_PANEL, buf, sizeof(buf));
}

int avrcp_get_capability()
{
  uint8_t buf[AVRCP_HEADER_LENGTH + 1];
  struct avrcp_header *pdu = (void *) buf;

  memset(buf, 0, sizeof(buf));
  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_GET_CAPABILITIES;
  pdu->params[0] = 0x03;
  pdu->params_len = htons(1);

  return avctp_send_vendordep(AVC_CTYPE_STATUS, AVC_SUBUNIT_PANEL, buf, sizeof(buf));
}

int avrcp_get_playstatus()
{
  uint8_t buf[AVRCP_HEADER_LENGTH];
  struct avrcp_header *pdu = (void *) buf;

  memset(buf, 0, sizeof(buf));
  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_GET_PLAY_STATUS;
  pdu->params_len = 0;

  return avctp_send_vendordep(AVC_CTYPE_STATUS, AVC_SUBUNIT_PANEL, buf, sizeof(buf));

}

int avrcp_get_attributes(uint32_t id)
{
  uint8_t buf[AVRCP_HEADER_LENGTH + 8 + 1 + 4 * 2];
  struct avrcp_header *pdu = (void *) buf;

  memset(buf, 0, sizeof(buf));
  set_company_id(pdu->company_id, IEEEID_BTSIG);

  pdu->pdu_id = AVRCP_GET_ELEMENT_ATTRIBUTES;
  pdu->params[8] = 2;
  net_store_32(pdu->params, 9, AVRCP_MEDIA_ATTRIBUTE_TITLE);
  net_store_32(pdu->params, 13, AVRCP_MEDIA_ATTRIBUTE_ARTIST);
//  net_store_32(pdu->params, 17, AVRCP_MEDIA_ATTRIBUTE_DURATION);
  pdu->params_len = htons(17);

  return avctp_send_vendordep(AVC_CTYPE_STATUS, AVC_SUBUNIT_PANEL, buf, sizeof(buf));
}