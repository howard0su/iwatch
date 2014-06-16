/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * @(#)$Id: watchdog.c,v 0.1 2014/02/17 17:40:41 Eric Fan1 Exp $
 */
 
#include "contiki.h"
#include "dev/watchdog.h"

#include <stdio.h>
#include "grlib/grlib.h"
#include "sys/process.h"

static int counter = 0;

#define PRINT_STACK_ON_REBOOT 2

void watchdog_init(void)
{
	void watchdog_stop();
	WDOG_Init_TypeDef init =
	{
		.enable     = false,               	/* Start watchdog when init done */
		.debugRun   = false,              	/* WDOG not counting during debug halt */
		.em2Run     = true,               	/* WDOG counting when in EM2 */
		.em3Run     = true,               	/* WDOG counting when in EM3 */
		.em4Block   = false,              	/* EM4 can be entered */
		.swoscBlock = false,              	/* Do not block disabling LFRCO/LFXO in CMU */
		.lock       = false,              	/* Do not lock WDOG configuration (if locked, reset needed to unlock) */
		.clkSel     = wdogClkSelLFXO,   	/* Select the 32.768kHZ LFXO oscillator */
		.perSel     = wdogPeriod_64k,      	/* Set the watchdog period to 65537 clock periods (ie ~2 seconds)*/
		
	};
	
	/* Initializing watchdog with choosen settings */
  	WDOG_Init(&init);
}

void watchdog_start(void)
{
	counter--;
	if(counter == 0)
	{	
		/* Enabling watchdog, since it was not enabled during initialization */	
  		WDOG_Enable(true);
  	}	
}
void watchdog_periodic(void)
{
	WDOG_Feed();
}
void watchdog_stop(void)
{
	counter++;
	if(counter == 1)
	{	
		/* Disabling watchdog, False to disable watchdog. */	
  		WDOG_Enable(false);
  	}		
}

void watchdog_reboot(void)
{
    	/* Write to the Application Interrupt/Reset Command Register to reset
     	 * the EFM32. See section 9.3.7 in the reference manual. */	
	SCB->AIRCR = 0x05FA0004;
} 
 
 