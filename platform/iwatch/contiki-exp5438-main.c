/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: contiki-z1-main.c,v 1.4 2010/08/26 22:08:11 nifi Exp $
 */

#include "contiki.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "dev/flash.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/uart1.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "sys/autostart.h"
#include "sys/profile.h"
#include "sys/ctimer.h"

#include "lcd.h"
#include "node-id.h"

/*--------------------------------------------------------------------------*/

#include "dev/button-sensor.h"
SENSORS(&button_sensor);

uint8_t bluetooth_uart_active = 1;

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
print_processes(struct process * const processes[])
{
  /*  const struct process * const * p = processes;*/
  printf("Starting");
  while(*processes != NULL) {
    printf(" %s", (*processes)->name);
    processes++;
  }
  putchar('\n');
}

/*--------------------------------------------------------------------------*/
int
main(int argc, char **argv)
{
  /*
   * Initalize hardware.
   */
  msp430_cpu_init();
  clock_init();
  
  uart1_init(BAUD2UBR(115200)); /* Must come before first printf */

  /* xmem_init(); */
  
  PRINTF(CONTIKI_VERSION_STRING "\n");
  /*
   * Hardware initialization done!
   */
  
  /* Restore node id if such has been stored in external mem */

  //  node_id_restore();
#ifdef NODEID
  node_id = NODEID;

#ifdef BURN_NODEID
  flash_setup();
  flash_clear(0x1800);
  flash_write(0x1800, node_id);
  flash_done();
#endif /* BURN_NODEID */
#endif /* NODE_ID */

  if(node_id == 0) {
    node_id = *((unsigned short *)0x1800);
  }

   /*
   * Initialize Contiki and our processes.
   */
  process_init();
  lcd_init();
  ctimer_init();

  process_start(&etimer_process, NULL);

  if(node_id > 0) {
    PRINTF("Node id %u.\n", node_id);
  } else {
    PRINTF("Node id not set.\n");
  }

  process_start(&sensors_process, NULL);
  //SENSORS_ACTIVATE(button_sensor);

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  print_processes(autostart_processes);
  autostart_start(autostart_processes);
  
  P5DIR |= BIT0;
  P5OUT |= BIT0;
  /*
   * This is the scheduler loop.
   */
  watchdog_start();
  watchdog_stop(); /* Stop the wdt... */
  while(1) {
    int r;
    do {
      /* Reset watchdog. */
      watchdog_periodic();
      r = process_run();
    } while(r > 0);

    /*
     * Idle processing.
     */
    int s = splhigh();          /* Disable interrupts. */
    /* uart1_active is for avoiding LPM3 when still sending or receiving */
    if(process_nevents() != 0) {
      splx(s);                  /* Re-enable interrupts. */
    } else {
      static unsigned long irq_energest = 0;

      P5OUT &= ~BIT0;
      /* Re-enable interrupts and go to sleep atomically. */
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_ON(ENERGEST_TYPE_LPM);
      /* We only want to measure the processing done in IRQs when we
         are asleep, so we discard the processing time done when we
         were awake. */
      energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
      watchdog_stop();

      if (uart1_active() || bluetooth_uart_active) // todo: move this logic to lpm module
      {
        _BIS_SR(GIE | CPUOFF);             /* LPM0 sleep. This
                                              statement will block
                                              until the CPU is
                                              woken up by an
                                              interrupt that sets
                                              the wake up flag. */
      }
      else
      {
        _BIS_SR(GIE | SCG0 | SCG1 | CPUOFF); /* LPM3 sleep. This
                                              statement will block
                                              until the CPU is
                                              woken up by an
                                              interrupt that sets
                                              the wake up flag. */
      }

      /* We get the current processing time for interrupts that was
         done during the LPM and store it for next time around.  */
      dint();
      irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
      eint();
      watchdog_start();
      ENERGEST_OFF(ENERGEST_TYPE_LPM);
      ENERGEST_ON(ENERGEST_TYPE_CPU);
      P5OUT |= BIT0;
    }
  }
}
/*---------------------------------------------------------------------------*/
