/*
 * Copyright (c) 2014. Viewcooper Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 * $Id: rtimer-arch.c,v 0.1 2014/02/16 16:03:59 Eric Fan Exp $
 */
/*---------------------------------------------------------------------------*/
/**
* \file
*			Real-timer specific implementation for EFM32.
* 
*
*/
/*---------------------------------------------------------------------------*/
/******************************************************************************
The Contiki kernel does not provide support for timed events.
Rather, an application that wants to use timers needs to explicitly use the timer library.
The timer library provides functions for setting, resetting and restarting timers, 
and for checking if a timer has expired. 
An application must "manually" check if its timers have expired; this is not done automatically.
A timer is declared as a struct timer and all access to the timer is made by
a pointer to the declared timer.
*******************************************************************************/
#include "contiki.h"
#include "contiki-conf.h"
#include "sys/energest.h"
#include "sys/rtimer.h"
#include "dev/watchdog.h"
#define DEBUG 0
#if DEBUG
#include <stdio.h>


#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* 13761 Hz -> 14Mhz (clock frequency) / 1024 (prescaler)
   Setting TOP to 27342 results in an overflow each 2 seconds */
#define TOP 27342

static uint32_t time_h = 0;  // Most significant bits of the current time.

// time of the next rtimer event. Initially is set to the max value.
static rtimer_clock_t next_rtimer_time = 0;

/*---------------------------------------------------------------------------*/
void TIMER1_IRQHandler(void)
{   
	
  	if (TIMER1->IF & TIMER_IF_OF)
  	{  	// Overflow event.         		    
  		time_h++;
  		/* Clear flag for TIMER0 overflow interrupt */
  		TIMER_IntClear(TIMER1, TIMER_IF_OF);
  		
 		ENERGEST_ON(ENERGEST_TYPE_IRQ);
  		watchdog_start();	
  		
  		rtimer_run_next();

  		if(process_nevents() > 0) 
  		{
#ifdef NOTYET  			
    			LPM4_EXIT;
#endif    			
  		}

  		watchdog_stop();

  		ENERGEST_OFF(ENERGEST_TYPE_IRQ);

	}  
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{

	/* Enable clock for TIMER0 module */
  	CMU_ClockEnable(cmuClock_TIMER1, true);
	/*1. Set initialize value to TIMER0*/
	/* Select TIMER0 parameters */  
  	TIMER_Init_TypeDef timerInit =
  	{
		.enable     = true, 
		.debugRun   = true, 
		.prescale   = timerPrescale1024, 
		.clkSel     = timerClkSelHFPerClk, 
		.fallAction = timerInputActionNone, 
		.riseAction = timerInputActionNone, 
		.mode       = timerModeUp, 
		.dmaClrAct  = false, 
		.quadModeX4 = false, 
		.oneShot    = false, 
		.sync       = false, 
  	};
  	
  	/* Enable overflow interrupt */
  	TIMER_IntEnable(TIMER1, TIMER_IF_OF);
  	
  	/* Enable TIMER0 interrupt vector in NVIC */
  	NVIC_EnableIRQ(TIMER1_IRQn);  	  	
	
	/* Set TIMER Top value */
  	TIMER_TopSet(TIMER1, TOP);  	
  	
	//2. Enable TIMER0
	/* Configure TIMER */
  	TIMER_Init(TIMER1, &timerInit);	
}
/*---------------------------------------------------------------------------*/
#ifdef NOTYET
void rtimer_arch_disable_irq(void)
{
  	ATOMIC(
  		saved_TIM1CFG = INT_TIM1CFG;
  		INT_TIM1CFG = 0;
  	)
}
/*---------------------------------------------------------------------------*/
void rtimer_arch_enable_irq(void)
{
  	INT_TIM1CFG = saved_TIM1CFG;
}
#endif
/*---------------------------------------------------------------------------*/
rtimer_clock_t rtimer_arch_now(void)
{
	return (rtimer_clock_t)TIMER_CounterGet(TIMER1);
}

/*---------------------------------------------------------------------------*/

void
rtimer_arch_schedule(rtimer_clock_t t)
{  
   	TIMER_TopSet(TIMER1, t);   	
}

