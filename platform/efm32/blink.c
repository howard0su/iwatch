/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * $Id: blink.c,v 1.1 2008/01/23 20:34:37 adamdunkels Exp $
 */

/**
 * \file
 *         A quick program that blinks the LEDs
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "contiki-conf.h"
#include "dev/leds.h"
#include "backlight.h"

extern void READ_MPU3050(void);
/*---------------------------------------------------------------------------*/
PROCESS(blink_process, "Blink");
AUTOSTART_PROCESSES(&blink_process);
//struct process * const autostart_processes[] = {&blink_process, NULL}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(blink_process, ev, data)
{
  	PROCESS_EXITHANDLER(goto exit;)
  	PROCESS_BEGIN();

  	while(1) 
  	{
    		static struct etimer et;
    		etimer_set(&et, CLOCK_SECOND*2);
    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    		BSP_LedToggle(0);
    		motor_on(6, CLOCK_SECOND);
    		backlight_on(6, CLOCK_SECOND);
    		READ_MPU3050();    		
    		//BSP_LedSet(1);
    		//leds_on(LEDS_ALL);
//    		etimer_set(&et, CLOCK_SECOND);
//    		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
//    		BSP_LedClear(0);
    		//BSP_LedToggle(1);
    		//BSP_LedSet(0);
    		//leds_off(LEDS_ALL);
  	}

 	exit:
 	BSP_LedSet(0);
  	//leds_off(LEDS_ALL);
  	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
