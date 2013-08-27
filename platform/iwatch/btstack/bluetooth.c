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
#define DEVICENAME "Kreyos %02X%02X"

#include "att.h"

#include "bluetooth.h"

extern void deviceid_init();
extern void spp_init();
extern void sdpc_open(const bd_addr_t remote_addr);

static att_connection_t att_connection;
static uint16_t         att_response_handle = 0;
static uint16_t         att_response_size   = 0;
static uint8_t          att_response_buffer[28];

static bd_addr_t currentbd;

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

static uint16_t handle_audio = 0;
static bd_addr_t host_addr;

// Bluetooth logic
static void packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
  bd_addr_t event_addr;

  // handle events, ignore data
  if (packet_type != HCI_EVENT_PACKET)
  {
    return;
  }

  switch (packet[0]) {
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
        process_post(ui_process, EVENT_BT_STATUS, (void*)BT_CONNECTED);
      } else {
        process_post(ui_process, EVENT_BT_STATUS, (void*)BT_DISCONNECTED);
      }
      break;
    }
  case HCI_EVENT_CONNECTION_REQUEST:
    {
      log_info("connection request\n");
      break;
    }
  case HCI_EVENT_CONNECTION_COMPLETE:
  {
    if (packet[11] == 2)
    {
      handle_audio = READ_BT_16(packet, 3);
      codec_wakeup();
    }
    break;
  }
  case HCI_EVENT_DISCONNECTION_COMPLETE:
    {
      att_response_handle =0;
      att_response_size = 0;

      if (READ_BT_16(packet, 3) == handle_audio)
      {
        handle_audio = 0;
        codec_shutdown();
      }
      // restart advertising
      // hci_send_cmd(&hci_le_set_advertise_enable, 1);
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
  case HCI_EVENT_IO_CAPABILITY_REQUEST:
    {
      log_info("IO_CAPABILITY_REQUEST\n");
      bt_flip_addr(event_addr, &packet[2]);
      hci_send_cmd(&hci_io_capability_request_reply, &event_addr, 0x01, 0x00, 0x00);
      break;
    }
  case HCI_EVENT_USER_CONFIRMATION_REQUEST:
    {
      bt_flip_addr(event_addr, &packet[2]);
      uint32_t value = READ_BT_32(packet, 8);
      log_info("USER_CONFIRMATION_REQUEST %lx\n", value);
      hci_send_cmd(&hci_user_confirmation_request_reply, &event_addr);
      break;
    }
  case HCI_EVENT_LINK_KEY_NOTIFICATION:
    {
      // new device is paired
      bt_flip_addr(event_addr, &packet[2]);
      memcpy(&currentbd, event_addr, sizeof(currentbd));
      //sdpc_open(event_addr);
      break;
    }
  case BTSTACK_EVENT_DISCOVERABLE_ENABLED:
    {
      if (packet[2])
      {
        hci_send_cmd(&hci_le_set_advertise_enable, 1);
      }
      else
      {
        hci_send_cmd(&hci_le_set_advertise_enable, 0);
      }
      break;
    }
  }
}

static void init_packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
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
      log_info("Start initialize bluetooth chip!\n");
      hci_send_cmd(&hci_read_local_supported_features);
    }
    break;
  case HCI_EVENT_COMMAND_COMPLETE:
    {
      if (COMMAND_COMPLETE_EVENT(packet, hci_read_bd_addr)){
        bt_flip_addr(host_addr, &packet[6]);
        log_info("BD ADDR: %s\n", bd_addr_to_str(host_addr));
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_read_local_supported_features)){
        log_info("Local supported features: %04lX%04lX\n", READ_BT_32(packet, 10), READ_BT_32(packet, 6));
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
        hci_send_cmd(&hci_vs_write_sco_config, 0x00, 120, 720, 0x01);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_vs_write_sco_config)){
        hci_send_cmd(&hci_vs_write_codec_config, 2048, 0, (uint32_t)8000, 0, 1, 0, 0,
                       16, 1, 0, 16, 1, 1, 0,
                       16, 40, 0, 16, 40, 1, 0
                       );
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_vs_write_codec_config)){
        char buf[20];
        sprintf(buf, DEVICENAME, host_addr[4], host_addr[5]);
        hci_send_cmd(&hci_write_local_name, buf);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_local_name)) {
        hci_send_cmd(&hci_write_class_of_device, 0x7C0704);
        break;
      }
      else if (COMMAND_COMPLETE_EVENT(packet, hci_write_class_of_device)) {
        process_post(ui_process, EVENT_BT_STATUS, (void*)BT_INITIALIZED);
        l2cap_register_packet_handler(packet_handler);
      }
      break;
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
  l2cap_register_packet_handler(init_packet_handler);
  l2cap_register_fixed_channel(att_packet_handler, L2CAP_CID_ATTRIBUTE_PROTOCOL);

  // init RFCOMM
  rfcomm_init();

  // set up ATT
  att_set_db(profile_data);
  att_set_write_callback(att_write_callback);
  //att_dump_attributes();
  att_connection.mtu = 100;

  // init SDP, create record for SPP and register with SDP
  sdp_init();

  // register device id record
  //deviceid_init();

  spp_init();

  avctp_init();
  avrcp_init();

  hfp_init();

  mns_init();
}

void bluetooth_init()
{
  // enable power
  OECLKDIR |= OECLKBIT;
  OECLKOUT &= ~OECLKBIT;

  OEHCIDIR |= OEHCIBIT;
  OEHCIOUT &= ~OEHCIBIT;

  btstack_setup();

  codec_init(); // init codec

  process_start(&bluetooth_process, NULL);
}

void bluetooth_shutdown()
{
  BT_SHUTDOWN_OUT &= ~BT_SHUTDOWN_BIT;  // = 1 - Active low

  process_exit(&bluetooth_process);

  // notify UI that we are shutdown
  process_post(ui_process, EVENT_BT_STATUS, (void*)BT_SHUTDOWN);
}

void bluetooth_discoverable(uint8_t onoff)
{
  hci_discoverable_control(onoff);
}

uint8_t bluetooth_paired()
{
  return 1;
}

bd_addr_t* bluetooth_paired_addr()
{
  return &currentbd;
}

const char* bluetooth_address()
{
  return bd_addr_to_str(host_addr);
}
