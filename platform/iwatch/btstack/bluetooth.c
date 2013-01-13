#include "contiki.h"
#include "sys/ctimer.h"

PROCESS(bluetooth_process, "Bluetooth process");

#include <stdio.h> /* For printf() */
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
#include "config.h"

static uint8_t   rfcomm_channel_nr = 1;
static uint16_t  rfcomm_channel_id;
static uint8_t   spp_service_buffer[110];

enum STATE {INIT, W4_LOCAL_NAME, W4_CONNECTION, W4_CHANNEL_COMPLETE, ACTIVE} ;
enum STATE state = INIT;

// Bluetooth logic
//static void packet_handler (void * connection, uint8_t packet_type, uint8_t *packet, uint16_t size)
static void packet_handler (void * connection, uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    bd_addr_t event_addr;
    uint8_t   rfcomm_channel_nr;
    uint16_t  mtu;
    uint8_t event = packet[0];
    
    // handle events, ignore data
    if (packet_type != HCI_EVENT_PACKET) return;
    
    switch(state){
        case INIT:
            // bt stack activated, get started - set local name
            if (packet[2] == HCI_STATE_WORKING) {
                hci_send_cmd(&hci_write_local_name, "BlueMSP-Demo");
                state = W4_LOCAL_NAME;
            }
            break;

        case W4_LOCAL_NAME:
            if ( COMMAND_COMPLETE_EVENT(packet, hci_write_local_name) ) {
                state = ACTIVE;
            }
            break;

        case ACTIVE:
        case W4_CONNECTION:
            switch (event) {
                case HCI_EVENT_PIN_CODE_REQUEST:
                    // inform about pin code request
                    printf("Pin code request - using '0000'\n\r");
                    bt_flip_addr(event_addr, &packet[2]);
                    hci_send_cmd(&hci_pin_code_request_reply, &event_addr, 4, "0000");
                    break;
                
                case RFCOMM_EVENT_INCOMING_CONNECTION:
                    // data: event (8), len(8), address(48), channel (8), rfcomm_cid (16)
                    bt_flip_addr(event_addr, &packet[2]); 
                    rfcomm_channel_nr = packet[8];
                    rfcomm_channel_id = READ_BT_16(packet, 9);
                    printf("RFCOMM channel %u requested for %s\n\r", rfcomm_channel_nr, bd_addr_to_str(event_addr));
                    rfcomm_accept_connection_internal(rfcomm_channel_id);
                    state = W4_CHANNEL_COMPLETE;
                    break;
                default:
                    break;
            }
        
        case W4_CHANNEL_COMPLETE:
                if ( event != RFCOMM_EVENT_OPEN_CHANNEL_COMPLETE ) break;
                
                // data: event(8), len(8), status (8), address (48), server channel(8), rfcomm_cid(16), max frame size(16)
                if (packet[2]) {
                    printf("RFCOMM channel open failed, status %u\n\r", packet[2]);
                } else {
                    rfcomm_channel_id = READ_BT_16(packet, 12);
                    mtu = READ_BT_16(packet, 14);
                    printf("\n\rRFCOMM channel open succeeded. New RFCOMM Channel ID %u, max frame size %u\n\r", rfcomm_channel_id, mtu);
                    state = ACTIVE;
                }
                break;
#if 0
        case ACTIVE:
            if (event != RFCOMM_EVENT_CHANNEL_CLOSED) break;
                
            rfcomm_channel_id = 0;
            state = W4_CONNECTION;
            break;
#endif
        default:
            break;
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
    l2cap_register_packet_handler(packet_handler);
    
    // init RFCOMM
    rfcomm_init();
    rfcomm_register_packet_handler(packet_handler);
    rfcomm_register_service_internal(NULL, rfcomm_channel_nr, 100);  // reserved channel, mtu=100

    // init SDP, create record for SPP and register with SDP
    sdp_init();
    memset(spp_service_buffer, 0, sizeof(spp_service_buffer));
    service_record_item_t * service_record_item = (service_record_item_t *) spp_service_buffer;
    sdp_create_spp_service( (uint8_t*) &service_record_item->service_record, 1, "SPP Counter");
    printf("SDP service buffer size: %u\n\r", (uint16_t) (sizeof(service_record_item_t) + de_get_len((uint8_t*) &service_record_item->service_record)));
    sdp_register_service_internal(NULL, service_record_item);
}


// set timer based on current time
void run_loop_set_timer(timer_source_t *a, uint32_t timeout_in_ms)
{
  printf("not implement: run_loop_set_timer\n");
}

// add/remove timer_source
void run_loop_add_timer(timer_source_t *timer)
{
  printf("not implement: run_loop_add_timer\n");
}

int  run_loop_remove_timer(timer_source_t *timer)
{
  printf("not implement: run_loop_remove_timer\n");
  return 1;
}

// add/remove data_source
static linked_list_t data_sources;

void run_loop_add_data_source(data_source_t *ds)
{
  linked_list_add(&data_sources, (linked_item_t *) ds);
}

int  run_loop_remove_data_source(data_source_t *ds)
{
  return linked_list_remove(&data_sources, (linked_item_t *) ds);
}


PROCESS_THREAD(bluetooth_process, ev, data)
{
    PROCESS_BEGIN();

    btstack_setup();
    
    // turn on!
    hci_power_control(HCI_POWER_ON);
    // make discoverable
    hci_discoverable_control(1);
    
    while(1)
    {
      PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
      
      // process data sources
      data_source_t *next;
      data_source_t *ds;
      for (ds = (data_source_t *) data_sources; ds != NULL ; ds = next){
          next = (data_source_t *) ds->item.next; // cache pointer to next data_source to allow data source to remove itself
          ds->process(ds);
        }
    }
    
    PROCESS_END();
}
