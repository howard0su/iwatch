#include "contiki.h"
#include "sys/etimer.h"

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

    static struct etimer timer;
    // wait about one second for bluetooth to start
    etimer_set(&timer, CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

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
