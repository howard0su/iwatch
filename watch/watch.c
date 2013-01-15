/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 * $Id: hello-world.c,v 1.1 2006/10/02 21:46:46 adamdunkels Exp $
 */

/**
 * \file
 *         A very simple Contiki application showing how Contiki programs look
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "string.h"
#include "dev/leds.h"
#include "lib/print-stats.h"
#include "sys/clock.h"
#include "lib/sensors.h"
#include "button.h"

#include "hal_lcd.h"

   extern void mpu6050_init();
   extern void bluetooth_init(int);
   
#include <stdlib.h>
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);
/*---------------------------------------------------------------------------*/

/*
 * This process is the startup process.
 * It first shows the logo
 * Like the whole dialog intrufstture.
 */
PROCESS_THREAD(system_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  PROCESS_EXITHANDLER(goto exit);

  SENSORS_ACTIVATE(button_sensor);

  halLcdPrintXY("iWatch", 20, 70, WIDE_TEXT | HIGH_TEXT);  
  // give time to starts
  etimer_set(&et, CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  //mpu6050_init();

        bluetooth_init(2);

  
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);
    printf("Key Changed %d, %d, %d, %d\n", 
           button_sensor.value(0),
           button_sensor.value(1),
           button_sensor.value(2),
           button_sensor.value(3)
           );
      
  }
  
 exit:
  leds_off(LEDS_ALL);
  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
