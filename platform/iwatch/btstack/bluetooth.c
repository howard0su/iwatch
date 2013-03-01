/****************************************************************
*  Description: Implementation for Initialize for btstack
*    History:
*      Jun Su          2013/1/1        Created
*      Jun Su          2013/1/13       Support BT and BLE
*      Jun Su          2013/1/21       Make BT and BLE runs same time
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include "contiki.h"
#include "window.h"

#include <string.h>

#include "bt_control_cc256x.h"

#include <btstack/hci_cmds.h>
#include <btstack/run_loop.h>
#include <btstack/sdp_util.h>

#include "hci.h"
#include "l2cap.h"
#include "btstack_memory.h"
#include "remote_device_db.h"
#include "rfcomm.h"
#include "sdp.h"
#include "hfp.h"
#include "config.h"
#include "avctp.h"
#include "avrcp.h"

#include "debug.h"
#define DEVICENAME "iWatch"

static uint16_t   spp_service_buffer[55];

#include "att.h"

static att_connection_t att_connection;
static uint16_t         att_response_handle = 0;
static uint16_t         att_response_size   = 0;
static uint8_t          att_response_buffer[28];

static void att_try_respond(void){
  if (!att_response_size) return;
  if (!att_response_handle) return;
  if (!hci_can_send_packet_now(HCI_ACL_DATA_PACKET)) return;

  // update state before sending packet
  uint16_t size = att_response_size;
  att_response_size = 0;
  l2cap_send_connectionless(att_response_handle, L2CAP_CID_ATTRIBUTE_PROTOCOL, att_response_buffer, size);
}


static void att_packet_handler(uint8_t packet_type, uint16_t handle, uint8_t *packet, uint16_t size){
  if (packet_type != ATT_DATA_PACKET) return;

  att_response_handle = handle;
  att_response_size = att_handle_request(&att_connection, packet, size, att_response_buffer);
  att_try_respond();
}

// test profile
#include "profile.h"

// write requests
static void att_write_callback(uint16_t handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size, signature_t * signature){
  log_info("WRITE Callback, handle %04x\n", handle);
  switch(handle){
  case 0x000b:
    buffer[buffer_size]=0;
    printf("New text: %s\n", buffer);
    break;
  case 0x000d:
    printf("New value: %u\n", buffer[0]);
    break;
  }
}



#define AT_BRSF "AT+BRSF=4\r"


// Bluetooth logic
static void packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  bd_addr_t event_addr;
  const uint8_t adv_data[] = { 02, 01, 05,   03, 02, 0xf0, 0xff };

  // handle events, ignore data
  if (packet_type != HCI_EVENT_PACKET)
  {
    return;
  }

  switch (packet[0]) {
  case BTSTACK_EVENT_STATE:
    // bt stack activated, get started - set local name
    if (packet[2] == HCI_STATE_WORKING) {
      printf("Start initialize bluetooth chip!\n");
      hci_send_cmd(&hci_read_local_supported_features);
    }
    break;

  case HCI_EVENT_COMMAND_COMPLETE:
    {
      if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)){
        bt_flip_addr(event_addr, &packet[6]);
        log_info("BD ADDR: %s\n", bd_addr_to_str(event_addr));
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_read_local_supported_features)){
        log_info("Local supported features: %04X%04X\n", READ_BT_32(packet, 10), READ_BT_32(packet, 6));
        hci_send_cmd(&hci_set_event_mask, 0xffffffff, 0x20001fff);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_set_event_mask)){
        hci_send_cmd(&hci_write_le_host_supported, 1, 1);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_le_host_supported)){
        hci_send_cmd(&hci_le_set_event_mask, 0xffffffff, 0xffffffff);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_event_mask)){
        hci_send_cmd(&hci_le_read_buffer_size);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_buffer_size)){
        log_info("LE buffer size: %u, count %u\n", READ_BT_16(packet,6), packet[8]);
        hci_send_cmd(&hci_le_read_supported_states);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_read_supported_states)){
        hci_send_cmd(&hci_le_set_advertising_parameters,  0x0400, 0x0800, 0, 0, 0, &event_addr, 0x07, 0);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertising_parameters)){
        hci_send_cmd(&hci_le_set_advertising_data, sizeof(adv_data), adv_data);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertising_data)){
        hci_send_cmd(&hci_le_set_scan_response_data, 10, adv_data);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_scan_response_data)){
        hci_send_cmd(&hci_le_set_advertise_enable, 1);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_le_set_advertise_enable)){
        hci_send_cmd(&hci_vs_write_codec_config, 64, 0x00, 1638, 0x0000, 0x00, 0x00, 0,
                       16, 0x00, 0x00, 0x08, 0x00, 0x00, 0,
                       16, 17, 0x00, 0x08, 0x00, 0x00, 0
                       );
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_vs_write_codec_config)){
        hci_send_cmd(&hci_write_voice_setting, 0x0060);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_voice_setting)){
        hci_send_cmd(&hci_write_local_name, DEVICENAME);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
        hci_send_cmd(&hci_write_class_of_device, 0x7C0704);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_class_of_device)) {
        process_post(ui_process, EVENT_BT_STATUS, (void*)BIT0);
      }
      break;
    }
  case HCI_EVENT_LE_META:
    switch (packet[2]) {
    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
      // reset connection MTU
      att_connection.mtu = 23;
      break;
    default:
      break;
    }
    break;

  case BTSTACK_EVENT_NR_CONNECTIONS_CHANGED:
    {
      if (packet[2]) {
        process_post(ui_process, EVENT_BT_STATUS, (void*)(BIT0 | BIT1));
      } else {
        process_post(ui_process, EVENT_BT_STATUS, (void*)BIT0);
      }
      break;
    }
  case HCI_EVENT_CONNECTION_REQUEST:
    {
      log_info("connection request\n");
      break;
    }
  case HCI_EVENT_DISCONNECTION_COMPLETE:
    {
      att_response_handle =0;
      att_response_size = 0;

      // restart advertising
      hci_send_cmd(&hci_le_set_advertise_enable, 1);
      break;
    }
  case HCI_EVENT_PIN_CODE_REQUEST:
    {
      // inform about pin code request
      log_info("Pin code request - using '0000'\n");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
      break;
    }
  }
}
#define SPP_CHANNEL 1
#define HFP_CHANNEL 6
static uint16_t hfp_channel_id = 0;
static uint16_t spp_channel_id = 0;

static void spp_handler(uint16_t channel, uint8_t packet_type, uint8_t *packet, uint16_t size)
{
}

static void rfcomm_app_packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  uint16_t rfcomm_id;

  if (packet_type == RFCOMM_DATA_PACKET)
  {
    if (channel == hfp_channel_id)
    {
      hfp_handler(hfp_channel_id, RFCOMM_DATA_PACKET, packet, size);
    }
    else if (channel == spp_channel_id)
    {
      spp_handler(spp_channel_id, RFCOMM_DATA_PACKET, packet, size);
    }
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
        switch(rfcomm_channel_nr)
        {
          case HFP_CHANNEL:
            {
              rfcomm_accept_connection_internal(rfcomm_id);
              hfp_channel_id = rfcomm_id;
              break;
            }
        case SPP_CHANNEL:
          {
            rfcomm_accept_connection_internal(rfcomm_id);
            spp_channel_id = rfcomm_id;
            break;
          }
        default:
          {
            log_info("unknow channel %d\n",  rfcomm_channel_nr);
            rfcomm_decline_connection_internal(rfcomm_id);
          }
        }
        break;
      }
    case RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE:
      {
        // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
        if (packet[2]) {
          log_info("RFCOMM channel open failed, status %u\n", packet[2]);
        } else {
          rfcomm_id = READ_BT_16(packet, 12);
          uint16_t mtu = READ_BT_16(packet, 14);
          log_info("RFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n", rfcomm_id, mtu);
          if (rfcomm_id == hfp_channel_id)
          {
            hfp_handler(hfp_channel_id, RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE, packet, size);
          }
        }
        break;
      }
    case RFCOMM_EVENT_CREDITS:
      {
        // data: event(8), len(8), rfcomm_cid(16), credits(8)
        rfcomm_id = READ_BT_16(packet, 2);
        uint8_t credits = packet[4];
        if (rfcomm_id == hfp_channel_id && credits > 0)
        {
          hfp_handler(hfp_channel_id, RFCOMM_EVENT_CREDITS, packet, size);
        }
        break;
      }
    case RFCOMM_EVENT_CHANNEL_CLOSED:
      {
        // data: event(8), len(8), rfcomm_cid(16)
        rfcomm_id = READ_BT_16(packet, 2);
        if (rfcomm_id == hfp_channel_id)
        {
          hfp_handler(hfp_channel_id, RFCOMM_EVENT_CHANNEL_CLOSED, packet, size);
          hfp_channel_id = 0;
        }
        break;
      }
    }
  }
}

static void btstack_setup(){
  /// GET STARTED with BTstack ///
  btstack_memory_init();

  // init HCI
  hci_transport_t    * transport = hci_transport_h4_dma_instance();
  bt_control_t       * control   = bt_control_cc256x_instance();
  hci_uart_config_t  * config    = hci_uart_config_cc256x_instance();
  remote_device_db_t * remote_db = (remote_device_db_t *) &remote_device_db_memory;
  hci_init(transport, config, control, remote_db);

  // use eHCILL
  bt_control_cc256x_enable_ehcill(1);

  // init L2CAP
  l2cap_init();
  l2cap_register_fixed_channel(att_packet_handler, L2CAP_CID_ATTRIBUTE_PROTOCOL);
  l2cap_register_packet_handler(packet_handler);

  // init RFCOMM
  rfcomm_init();
  rfcomm_register_packet_handler(rfcomm_app_packet_handler);
  rfcomm_register_service_internal(NULL, 1, 100);  // reserved channel, mtu=100

  // set up ATT
  att_set_db(profile_data);
  att_set_write_callback(att_write_callback);
  //att_dump_attributes();
  att_connection.mtu = 100;

  // init SDP, create record for SPP and register with SDP
  sdp_init();

  service_record_item_t * service_record_item;
  memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
  service_record_item = (service_record_item_t *) spp_service_buffer;
  sdp_create_spp_service( (uint8_t*) &service_record_item->service_record, 1, "iWatch Configure");
  log_info("SDP service buffer size: %u\n", (uint16_t) (sizeof(service_record_item_t) + de_get_len((uint8_t*) &service_record_item->service_record)));
  //de_dump_data_element(service_record_item->service_record);
  sdp_register_service_internal(NULL, service_record_item);

  hfp_init(HFP_CHANNEL);

  avctp_init();
  avrcp_init();
}

PROCESS_NAME(bluetooth_process);

void bluetooth_init()
{
  int x = splhigh();
  btstack_setup();
  splx(x);

  // set BT SHUTDOWN to 1 (active low)
  BT_SHUTDOWN_SEL &= ~BT_SHUTDOWN_BIT;  // = 0 - I/O
  BT_SHUTDOWN_DIR |=  BT_SHUTDOWN_BIT;  // = 1 - Output
  BT_SHUTDOWN_OUT &=  ~BT_SHUTDOWN_BIT;  // = 0

  // Enable ACLK to provide 32 kHz clock to Bluetooth module
  BT_ACLK_SEL |= BT_ACLK_BIT;
  BT_ACLK_DIR |= BT_ACLK_BIT;

  codec_init();

  process_start(&bluetooth_process, NULL);
  BT_SHUTDOWN_OUT |=  BT_SHUTDOWN_BIT;  // = 1 - Active low
}

void bluetooth_shutdown()
{
  BT_SHUTDOWN_OUT &=  BT_SHUTDOWN_BIT;  // = 1 - Active low

  process_exit(&bluetooth_process);

  // notify UI that we are shutdown
  process_post(ui_process, EVENT_BT_STATUS, 0);
}