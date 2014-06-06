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

/* 46875 Hz -> 48Mhz (clock frequency) / 1024 (prescaler)
   Setting TOP to 5860 results in an overflow each 1/8 seconds */
#define TOP 5859

/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
	dint();	
	LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP1);
	/* Enable interrupts. */
  	eint();
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
	return (0xffff - LETIMER_CounterGet(LETIMER0) );	
}

/*---------------------------------------------------------------------------*/

void
rtimer_arch_schedule(rtimer_clock_t t)
{ 
	LETIMER_CompareSet(LETIMER0, 1, t); 
}

