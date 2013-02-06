#include "contiki.h"

#include "hci.h"
#include "hfp.h"
#include "sdp.h"
#include "rfcomm.h"
#include "btstack/sdp_util.h"

#include <string.h>
#include "config.h"
#include "debug.h"

static enum
{
  INIT,
  SEND_BRSF,
  WAIT_BRSF,
  SEND_CIND0,
  WAIT_CIND0,
  SEND_CIND,
  WAIT_CIND,
  SEND_CMER,
  WAIT_OK,
  IDLE,
  WAIT_RESP,
  ERROR
}state = INIT;

static uint8_t   hpf_service_buffer[90];
void hfp_init(int channel)
{
  rfcomm_register_service_internal(NULL, channel, 100);  // reserved channel, mtu=100
  
  service_record_item_t * service_record_item;
  memset(hpf_service_buffer, 0, sizeof(hpf_service_buffer));
  service_record_item = (service_record_item_t *) hpf_service_buffer;
  sdp_create_hfp_service( (uint8_t*) &service_record_item->service_record, channel, "Headset");
  printf("HPF service buffer size: %u\n", (uint16_t) (sizeof(service_record_item_t) + de_get_len((uint8_t*) &service_record_item->service_record)));
  //de_dump_data_element(service_record_item->service_record);
  sdp_register_service_internal(NULL, service_record_item);
}

#define AT_BRSF  "AT+BRSF=4\r"
#define AT_CIND0 "AT+CIND=?\r"
#define AT_CIND  "AT+CIND?\r"
#define AT_CMER  "AT+CMER=3,0,0,1\r"

#define R_BRSF 1
#define R_CIND 2

static int parse_return(uint8_t* result, int* code)
{
  *code = 0;

  log_info("parse return: %s\n", result);
  if (strncmp(result, "\r\n+", 3) == 0)
  {
    // skip the cr/lf
    result+=3;

    if (strncmp(result, "BRSF", 4) == 0)
    {
      *code = R_BRSF;
    }
    else if (strncmp(result, "CIND", 4) == 0)
    {
      *code = R_CIND;
    }
    else if (strncmp(result, "OK\r\n", 4) == 0)
    {
      return 1;
    }

    while(*result != '\r' && *(result + 1) != '\n' && *result != '\0')
    {
      result++;
    }

    if (*result == '\0')
      return 2; // need more data

    result+=2;
  }
  
  if (strncmp(result, "\r\n", 2) == 0)
  {
    // skip the cr/lf
    result+=2;

    if (strncmp(result, "ERROR\r\n", 4) == 0)
    {
      return 0;
    }
    else if (strncmp(result, "OK\r\n", 4) == 0)
    {
      return 1;
    }
  }
  else 
    return 2;

  return 1;
}

static uint8_t textbuf[255];
static uint8_t textbufptr = 0;
void hfp_handler(int rfcomm_channel_id, int type, uint8_t *packet, uint16_t len)
{
  log_info("hfp_handler state %d event %d\n", state, type);
  switch(type)
  {
  case RFCOMM_DATA_PACKET:
    {
      int code;
      int ret;
      memcpy(textbuf + textbufptr, packet, len);
      textbufptr+=len;
      textbuf[textbufptr] = 0;
      ret = parse_return(textbuf, &code);
      log_info("parse_return ret : %d code: %d\n", ret, code);
      if (ret == 2)
      {
        break; // need more data
      }
      else if (ret == 1)
      {
        textbufptr = 0;
        if (state == WAIT_BRSF && code == R_BRSF)
        {
          rfcomm_send_internal(rfcomm_channel_id, AT_CIND0, sizeof(AT_CIND0));
          state = WAIT_CIND0;
          //rfcomm_grant_credits(rfcomm_channel_id, 1);
        }
        else if (state == WAIT_CIND0 && code == R_CIND)
        {
          rfcomm_send_internal(rfcomm_channel_id, AT_CIND, sizeof(AT_CIND));
          state = WAIT_CIND;
          //rfcomm_grant_credits(rfcomm_channel_id, 1);
        }
        else if (state == WAIT_CIND && code == R_CIND)
        {
          rfcomm_send_internal(rfcomm_channel_id, AT_CMER, sizeof(AT_CMER));
          state = WAIT_OK;
          //rfcomm_grant_credits(rfcomm_channel_id, 1);
        }
        else if (state == WAIT_OK)
        {
          state = IDLE;
          //rfcomm_grant_credits(rfcomm_channel_id, 1);
        }
        else state = ERROR;
      }
      break;
    }
  case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
    {
      if (state == INIT)
      {
        state = SEND_BRSF;
        rfcomm_grant_credits(rfcomm_channel_id, 1);
      }
      else
      {
        state = ERROR;
      }
      break;
    }

  case RFCOMM_EVENT_CREDITS:
    {
      if (state == SEND_BRSF)
      {
        rfcomm_send_internal(rfcomm_channel_id, AT_BRSF, sizeof(AT_BRSF));
        state = WAIT_BRSF;
      }
      else if (state == SEND_CIND0)
      {
        rfcomm_send_internal(rfcomm_channel_id, AT_BRSF, sizeof(AT_CIND0));
        state = WAIT_CIND0;
      }
      else if (state == SEND_CIND)
      {
        rfcomm_send_internal(rfcomm_channel_id, AT_BRSF, sizeof(AT_CIND));
        state = WAIT_CIND;
      }
      else if (state == SEND_CMER)
      {
        rfcomm_send_internal(rfcomm_channel_id, AT_BRSF, sizeof(SEND_CMER));
        state = WAIT_OK;
      }
      break;
    }
  case RFCOMM_EVENT_CHANNEL_CLOSED:
    {
      state = INIT;
      textbufptr = 0;
      break;
    }
  }
}