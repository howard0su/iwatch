#include <stdio.h>

#include "sdp.h"
#include "avrcp.h"
#include "avctp.h"
#include "btstack/sdp_util.h"

static uint8_t   avrcp_service_buffer[100];

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