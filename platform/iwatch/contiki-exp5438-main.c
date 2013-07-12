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
#include "sys/autostart.h"
#include "sys/profile.h"
#include "sys/ctimer.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include "node-id.h"
#include "power.h"
#include "battery.h"
#include "spiflash.h"
#include "btstack/bluetooth.h"
#include "backlight.h"
#include "window.h"

/*--------------------------------------------------------------------------*/
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

extern void mpu6050_init();
extern void button_init();
extern void I2C_Init();
extern void rtc_init();
extern void battery_init();

uint8_t SMCLK_NEED = 0;

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

  PRINTF("iWatch 0.10 build at " __TIME__ " " __DATE__ "\n");
  /*
  * Hardware initialization done!
  */

  /*
  * Initialize Contiki and our processes.
  */
  process_init();
  process_start(&etimer_process, NULL);
  ctimer_init();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  backlight_init();
  window_init();

  battery_init();
  button_init();
  rtc_init();
  I2C_Init();

  //codec_init();
  //ant_init();
    bluetooth_init();

//  if (!bluetooth_paired())
  {
    bluetooth_discoverable(1);
  }

  mpu6050_init();

  autostart_start(autostart_processes);

  SPI_FLASH_Init();

  /*
  * This is the scheduler loop.
  */
  SMCLK_NEED = 0;
  watchdog_start();

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

      /* Re-enable interrupts and go to sleep atomically. */
      ENERGEST_OFF(ENERGEST_TYPE_CPU);
      ENERGEST_ON(ENERGEST_TYPE_LPM);
      /* We only want to measure the processing done in IRQs when we
         are asleep, so we discard the processing time done when we
         were awake. */
      energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
      watchdog_stop();

      if (SMCLK_NEED)
      {
        __bis_SR_register(GIE | CPUOFF);
      }
      else
      {
      __bis_SR_register(GIE | CPUOFF | SCG0 | SCG1);
      }

      /* We get the current processing time for interrupts that was
         done during the LPM and store it for next time around.  */
      dint();
      irq_energest = energest_type_time(ENERGEST_TYPE_IRQ);
      eint();
      watchdog_start();
      ENERGEST_OFF(ENERGEST_TYPE_LPM);
      ENERGEST_ON(ENERGEST_TYPE_CPU);
    }
  }
}
/*---------------------------------------------------------------------------*/
