#include "contiki.h"

#include "hci.h"
#include "hfp.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include "config.h"
#include "debug.h"

#include "window.h"
#define HFP_CHANNEL 6

static enum
{
  INITIALIZING,
  WAIT_BRSF,
  WAIT_CIND0,
  WAIT_CIND,
  WAIT_CMEROK,
  WAIT_OK,
  IDLE,
  WAIT_RESP,
  ERROR
}state;

static uint16_t hfp_response_size;
static void*    hfp_response_buffer;
static uint16_t rfcomm_channel_id = 0;

static void hfp_try_respond(uint16_t rfcomm_channel_id){
    if (!hfp_response_size) return;
    if (!rfcomm_channel_id) return;

    printf("HFP: sending %s\n", hfp_response_buffer);
    // update state before sending packet (avoid getting called when new l2cap credit gets emitted)
    uint16_t size = hfp_response_size;
    hfp_response_size = 0;
    if (rfcomm_send_internal(rfcomm_channel_id, hfp_response_buffer, size) != 0)
    {
      // if error, we need retry
      printf("HFP: send failed.\n");
      hfp_response_size = size;
    }
}

static service_record_item_t hfp_service_record;
static const uint8_t   hfp_service_buffer[85] =
{
  0x36,0x00,0x52,0x09,0x00,0x00,0x0A,0x00,0x01,0x00,0x02,0x09,0x00,0x01,0x36,
  0x00,0x06,0x19,0x11,0x1E,0x19,0x12,0x03,0x09,0x00,0x04,0x36,0x00,0x0E,0x36,
  0x00,0x03,0x19,0x01,0x00,0x36,0x00,0x05,0x19,0x00,0x03,0x08,0x06,0x09,0x00,
  0x05,0x36,0x00,0x03,0x19,0x10,0x02,0x09,0x00,0x09,0x36,0x00,0x09,0x36,0x00,
  0x06,0x19,0x11,0x1E,0x09,0x01,0x06,0x09,0x01,0x00,0x25,0x07,0x48,0x65,0x61,
  0x64,0x73,0x65,0x74,0x09,0x03,0x11,0x09,0x00,0x08
};

static void hfp_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

static void sdp_create_hfp_service(uint8_t *service, int service_id, const char *name){

	uint8_t* attribute;
	de_create_sequence(service);

    // 0x0000 "Service Record Handle"
	de_add_number(service, DE_UINT, DE_SIZE_16, SDP_ServiceRecordHandle);
	de_add_number(service, DE_UINT, DE_SIZE_32, 0x10002);

	// 0x0001 "Service Class ID List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_ServiceClassIDList);
	attribute = de_push_sequence(service);
	{
		de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x111E );
		de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1203 );
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
	}
	de_pop_sequence(service, attribute);

    // 0x0005 "Public Browse Group"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_BrowseGroupList); // public browse group
	attribute = de_push_sequence(service);
	{
		de_add_number(attribute,  DE_UUID, DE_SIZE_16, 0x1002 );
	}
	de_pop_sequence(service, attribute);

	// 0x0009 "Bluetooth Profile Descriptor List"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_BluetoothProfileDescriptorList);
	attribute = de_push_sequence(service);
	{
		uint8_t *hfpProfile = de_push_sequence(attribute);
		{
			de_add_number(hfpProfile,  DE_UUID, DE_SIZE_16, 0x111E);
			de_add_number(hfpProfile,  DE_UINT, DE_SIZE_16, 0x0106);
		}
		de_pop_sequence(attribute, hfpProfile);
	}
	de_pop_sequence(service, attribute);

	// 0x0100 "ServiceName"
	de_add_number(service,  DE_UINT, DE_SIZE_16, 0x0100);
	de_add_data(service,  DE_STRING, strlen(name), (uint8_t *) name);

        // 0x0311 "SupportedFeatures"
	de_add_number(service,  DE_UINT, DE_SIZE_16, SDP_SupportedFeatures);
	de_add_number(service,  DE_UINT, DE_SIZE_16, 0x08); // Voice recognition
}


void hfp_init()
{
  rfcomm_register_service_internal(NULL, hfp_handler, HFP_CHANNEL, 100);  // reserved channel, mtu=100

  memset(&hfp_service_record, 0, sizeof(hfp_service_record));
  hfp_service_record.service_record = (uint8_t*)&hfp_service_buffer[0];
#if 0
  sdp_create_hfp_service( hfp_service_record.service_record, channel, "Headset");
  log_info("SDP service buffer size: %u\n", de_get_len(hfp_service_record.service_record));
  //de_dump_data_element(service_record_item->service_record);
  hexdump((void*)hfp_service_buffer, de_get_len(hfp_service_record.service_record));
#endif
  sdp_register_service_internal(NULL, &hfp_service_record);
  state = INITIALIZING;
}

void hfp_open(const bd_addr_t *remote_addr, uint8_t port)
{
  if (rfcomm_channel_id)
    return;

  rfcomm_create_channel_internal(NULL, hfp_handler, (bd_addr_t*)remote_addr, port);
  state = INITIALIZING;
}

#define AT_BRSF  "AT+BRSF=4\r"
#define AT_CIND0 "AT+CIND=?\r"
#define AT_CIND  "AT+CIND?\r"
#define AT_CMER  "AT+CMER=3,0,0,1\r"
#define AT_CLIP  "AT+CLIP=1\r"
#define AT_BVRA  "AT+BVRA=1\r"
#define AT_BTRH1 "AT+BTRH=1\r"
#define AT_BTRH2 "AT+BTRH=2\r"
#define AT_CHUP  "AT+CHUP\r"
#define AT_ATA   "ATA\r"

#define R_NONE 0
#define R_OK   0
#define R_BRSF 1
#define R_CIND 2
#define R_CIEV 3
#define R_RING 4
#define R_CLIP 5
#define R_BTRH 6
#define R_UNKNOWN 0xFE
#define R_ERROR 0xFF
#define R_CONTINUE 0xFC

static char* parse_return(char* result, int* code)
{
  char* ret;
  //log_info("parse return: %s\n", result);

  if (result[0] == '\r' && result[1] == '\n')
  {
    //skip empty line
    result += 2;
  }

  ret = result;

  if (strncmp(result, "+BRSF", 5) == 0)
  {
    *code = R_BRSF;
  }
  else if (strncmp(result, "+CIND", 5) == 0)
  {
    *code = R_CIND;
  }
  else if (strncmp(result, "+CIEV", 5) == 0)
  {
    *code = R_CIEV;
  }
  else if (strncmp(result, "+CLIP", 5) == 0)
  {
    *code = R_CLIP;
  }
  else if (strncmp(result, "+BTRH", 5) == 0)
  {
    *code = R_BTRH;
  }
  else if (strncmp(result, "OK", 2) == 0)
  {
    *code = R_OK;
  }
  else if (strncmp(result, "ERROR", 5) == 0)
  {
    *code = R_ERROR;
  }
  else if (result[0] == '\0')
  {
    *code = R_UNKNOWN;
    return NULL;
  }
  while (result[0] != '\r' || result[1] != '\n')
  {
    if (*result == '\0')
    {
      *code = R_CONTINUE;
      return ret;
    }
    result++;
  }

  if (result[0] == '\r' && result[1] == '\n')
  {
    ret = result + 2;
    *result = '\0';
  }

  return ret;
}

struct hfp_cind {
	uint8_t service;	/*!< whether we have service or not */
	uint8_t call;	/*!< call state */
	uint8_t callsetup;	/*!< bluetooth call setup indications */
	uint8_t callheld;	/*!< bluetooth call hold indications */
	uint8_t signal;	/*!< signal strength */
	uint8_t roam;	/*!< roaming indicator */
	uint8_t battchg;	/*!< battery charge indicator */
}cind_map;
static uint8_t cind_index[16];
static uint8_t cind_state[16];


static void handle_CIEV(char *buf)
{
  uint8_t ind, value;
  // handle +CIEV: 3,0
  // handle +CIEV: 3,1

  int i, state;
  size_t s;
  char *indicator = NULL;

  log_info("%s\n", buf);

  /* parse current state of all of our indicators.  The list is in the
  * following format:
  * +CIND: 1,0,2,0,0,0,0
  */
  state = 0;
  for (i = 0; buf[i] != '\0'; i++) {
    switch (state) {
    case 0: /* search for start of the status indicators (a space) */
      if (buf[i] == ' ') {
        state++;
      }
      break;
    case 1: /* mark this indicator */
      indicator = &buf[i];
      state++;
      break;
    case 2: /* search for the start of the next indicator (a comma) */
      if (buf[i] == ',') {
        buf[i] = '\0';
        ind = atoi(indicator);
        state = 1;
      }
      break;
    }
  }

  value = atoi(indicator);

  log_info("CIEV: ind:%d index:%d value:%d\n", ind, cind_index[ind], value);
  cind_state[cind_index[ind]] = value;
  process_post(ui_process, EVENT_RING, (void*)(cind_index[ind] << 8 | value));
 
  return;
}

static void handle_RING()
{
  process_post(ui_process, EVENT_RING, NULL);
}

static void handle_CLIP(char* buf)
{
  char phone[20];
  if (sscanf("\r\n+CLIP: \"%s\"", phone))
  {
    log_info("CLIP: %s\n", phone);
    process_post_synch(ui_process, EVENT_RING_NUM, phone);
  }
}

static void handle_CIND0(char* buf)
{
  int i, state, group;
  size_t s;
  char *indicator = NULL, *values;

  /* parse the indications list.  It is in the follwing format:
  * +CIND: ("ind1",(0-1)),("ind2",(0-5))
  */
  group = 0;
  state = 0;
  s = strlen(buf);
  for (i = 0; i < s; i++) {
    switch (state) {
    case 0: /* search for start of indicator block */
      if (buf[i] == '(') {
        group++;
        state++;
      }
      break;
    case 1: /* search for '"' in indicator block */
      if (buf[i] == '"') {
        state++;
      }
      break;
    case 2: /* mark the start of the indicator name */
      indicator = &buf[i];
      state++;
      break;
    case 3: /* look for the end of the indicator name */
      if (buf[i] == '"') {
        buf[i] = '\0';
        state++;
      }
      break;
    case 4: /* find the start of the value range */
      if (buf[i] == '(') {
        state++;
      }
      break;
    case 5: /* mark the start of the value range */
      values = &buf[i];
      state++;
      break;
    case 6: /* find the end of the value range */
      if (buf[i] == ')') {
        buf[i] = '\0';
        state++;
      }
      break;
    case 7: /* process the values we found */
      if (group < sizeof(cind_index)) {
        if (!strcmp(indicator, "service")) {
          cind_map.service = group;
          cind_index[group] = HFP_CIND_SERVICE;
        } else if (!strcmp(indicator, "call")) {
          cind_map.call = group;
          cind_index[group] = HFP_CIND_CALL;
        } else if (!strcmp(indicator, "callsetup")) {
          cind_map.callsetup = group;
          cind_index[group] = HFP_CIND_CALLSETUP;
        } else if (!strcmp(indicator, "call_setup")) { /* non standard call setup identifier */
          cind_map.callsetup = group;
          cind_index[group] = HFP_CIND_CALLSETUP;
        } else if (!strcmp(indicator, "callheld")) {
          cind_map.callheld = group;
          cind_index[group] = HFP_CIND_CALLHELD;
        } else if (!strcmp(indicator, "signal")) {
          cind_map.signal = group;
          cind_index[group] = HFP_CIND_SIGNAL;
        } else if (!strcmp(indicator, "roam")) {
          cind_map.roam = group;
          cind_index[group] = HFP_CIND_ROAM;
        } else if (!strcmp(indicator, "battchg")) {
          cind_map.battchg = group;
          cind_index[group] = HFP_CIND_BATTCHG;
        } else {
          cind_index[group] = HFP_CIND_UNKNOWN;
          log_info("ignoring unknown CIND indicator '%s'\n", indicator);
        }
      } else {
        log_info("can't store indicator %d (%s), we only support up to %d indicators", group, indicator, (int) sizeof(cind_index));
      }

      state = 0;
      break;
    }
  }

  return;
}

/*!
 * \brief Read the result of the AT+CIND? command.
 * \param hfp an hfp_pvt struct
 * \param buf the buffer to parse (null terminated)
 * \note hfp_send_cind_test() and hfp_parse_cind_test() should be called at
 * least once before this function is called.
 */
static int handle_CIND(char *buf)
{
  int i, state, group;
  size_t s;
  char *indicator = NULL;

  /* parse current state of all of our indicators.  The list is in the
  * following format:
  * +CIND: 1,0,2,0,0,0,0
  */
  group = 0;
  state = 0;
  for (i = 0; buf[i] != '\0'; i++) {
    switch (state) {
    case 0: /* search for start of the status indicators (a space) */
      if (buf[i] == ' ') {
        group++;
        state++;
      }
      break;
    case 1: /* mark this indicator */
      indicator = &buf[i];
      state++;
      break;
    case 2: /* search for the start of the next indicator (a comma) */
      if (buf[i] == ',') {
        buf[i] = '\0';
        cind_state[group] = atoi(indicator);
        group++;
        state = 1;
      }
      break;
    }
  }

  /* store the last indicator */
  if (state == 2)
  {
    cind_state[group] = atoi(indicator);
  }
  return 0;
}

static void hfp_state_handler(int code, char* buf)
{
  if (state == WAIT_BRSF && code == R_BRSF)
  {
    hfp_response_buffer = AT_CIND0;
    hfp_response_size = sizeof(AT_CIND0);
    state = WAIT_CIND0;
    hfp_try_respond(rfcomm_channel_id);
  }
  else if (state == WAIT_CIND0 && code == R_CIND)
  {
    handle_CIND0(buf);
    hfp_response_buffer = AT_CIND;
    hfp_response_size = sizeof(AT_CIND);
    state = WAIT_CIND;
    hfp_try_respond(rfcomm_channel_id);
  }
  else if (state == WAIT_CIND && code == R_CIND)
  {
    handle_CIND(buf);
    hfp_response_buffer = AT_CMER;
    hfp_response_size = sizeof(AT_CMER);
    state = WAIT_CMEROK;
    hfp_try_respond(rfcomm_channel_id);
  }
  else if (state == WAIT_CMEROK && code == R_OK)
  {
    hfp_response_buffer = AT_CLIP;
    hfp_response_size = sizeof(AT_CLIP);
    hfp_try_respond(rfcomm_channel_id);
    state = WAIT_OK;
  }
  else if (code == R_CIEV)
  {
    handle_CIEV(buf);
  }
  else if (code == R_RING)
  {
    handle_RING();
  }
  else if (code == R_CLIP)
  {
    handle_CLIP(buf);
  }
  else if (code == R_OK || code == R_ERROR)
  {
  }
  else
  {
    state = ERROR;
  }
}

static char textbuf[255];
static uint8_t textbufptr = 0;
static void hfp_handler(uint8_t type, uint16_t channelid, uint8_t *packet, uint16_t len)
{
  log_info("HFP: state %d event %d[%d]\n", state, type, packet[0]);
  switch(type)
  {
  case RFCOMM_DATA_PACKET:
    {
      int code;
      char* next;
      memcpy(textbuf + textbufptr, packet, len);
      textbufptr+=len;
      textbuf[textbufptr] = 0;
      //printf("HFP: recv so far: %s\n", textbuf);
      char* current = textbuf;
      do
      {
        next = parse_return(current, &code);
        if (code == R_CONTINUE)
        {
          if (textbuf != next)
          {
            textbufptr = strlen(next);
            memcpy(textbuf, next, textbufptr);
          }
          break; // need more data
        }
        else
        {
          textbufptr = 0;
          if (code != R_UNKNOWN)
          {
            hfp_state_handler(code, current);
            hfp_try_respond(rfcomm_channel_id);
          }
        }
        current = next;
      }while(current != NULL);

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
          log_info("HFP channel %u requested for %s\n", rfcomm_channel_nr, bd_addr_to_str(event_addr));
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
            state = WAIT_BRSF;
            hfp_response_buffer = AT_BRSF;
            hfp_response_size = sizeof(AT_BRSF);
            hfp_try_respond(rfcomm_channel_id);
          }
          else
          {
            state = ERROR;
          }
          break;
        }
      case RFCOMM_EVENT_CREDITS:
        {
          hfp_try_respond(rfcomm_channel_id);
          break;
        }
      case RFCOMM_EVENT_CHANNEL_CLOSED:
        {
          if (rfcomm_channel_id)
          {
            rfcomm_channel_id = 0;
          }
          state = INITIALIZING;
          textbufptr = 0;
          break;
        }
      }
    }
  }
}

uint8_t hfp_enable_voicerecog()
{
  if (hfp_response_size)
    return -1;

  hfp_response_buffer = AT_BVRA;
  hfp_response_size = sizeof(AT_BVRA);
  hfp_try_respond(rfcomm_channel_id);

  return 0;
}

uint8_t hfp_accept_call(uint8_t accept)
{
  if (hfp_response_size)
    return -1;

  if (accept)
  {
    hfp_response_buffer = AT_ATA;
    hfp_response_size = sizeof(AT_ATA);
    hfp_try_respond(rfcomm_channel_id);    
  }
  else
  {
    hfp_response_buffer = AT_CHUP;
    hfp_response_size = sizeof(AT_CHUP);
    hfp_try_respond(rfcomm_channel_id);
  }

  return 0;
}

