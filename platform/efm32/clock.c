/****************************************************************************
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
 * @(#)$Id: clock.c,v 0.1 2014/02/13 11:26:38 Eric Fan $
 *****************************************************************************/
/************************************************************************
 This file is the clock libery of Contiki OS
 The clock library is the interface between Contiki and the platform specific clock functionality.	
 The clock library defines a macro, CLOCK_SECOND, to convert seconds into the tick resolution of the platform. Typically this is 1-10 milliseconds, e.g. 4*CLOCK_SECOND could be 512. A 16 bit counter would thus overflow every 1-10 minutes. Platforms use the tick interrupt to maintain a long term count of seconds since startup.
 Platforms may also implement rtimers for greater time resolution and for real-time interrupts, These use a corresponding RTIMER_SECOND. 
 ************************************************************************/
 
/* Includes ------------------------------------------------------------------*/
//#include "board.h"

#include "contiki.h"
#include "dev/watchdog.h"


/***************************************************************************//**
 * @addtogroup SysTick_clock_source
 * @{
 ******************************************************************************/
#define SysTick_CLKSource_MASK          	((uint32_t)0x00000004)
#define SysTick_CLKSource_RTC		    	((uint32_t)0x00000000)
#define SysTick_CLKSource_HFCORECLK		((uint32_t)0x00000004)
#define IS_SYSTICK_CLK_SOURCE(SOURCE)		(((SOURCE) == SysTick_CLKSource_RTC) || \					
						((SOURCE) == SysTick_CLKSource_HFCORECLK))

//static volatile clock_time_t msTicks=0;						
static volatile clock_time_t count=0;

static unsigned int second_countdown = CLOCK_SECOND;										
static volatile unsigned long seconds;

#define INTERVAL (RTIMER_ARCH_SECOND / CLOCK_SECOND)										
#define MAX_TICKS (~((clock_time_t)0) / 2)

/* last_tar is used for calculating clock_fine, last_ccr might be better? */
static unsigned short last_tar = 0;
//static volatile uint32_t msTicks; /* counts 1ms timeTicks */
/***************************************************************************//**
 * @brief
 *   Configure the SysTick clock source
 *
 * @details
 *
 * @note
 *
 * @param[in] SysTick_CLKSource
 *	 Specifies the SysTick clock source.
 *
 * @arg SysTick_CLKSource_HCLK_Div8
 * 	 AHB clock divided by 8 selected as SysTick clock source.
 *
 * @arg SysTick_CLKSource_HCLK
 *	 AHB clock selected as SysTick clock source.
 ******************************************************************************/
static void SysTick_CLKSourceConfig(uint32_t SysTick_CLKSource)
{
	/* Check the parameters */
	EM_ASSERT(IS_SYSTICK_CLK_SOURCE(SysTick_CLKSource)); 
	
	uint32_t ctrl = SysTick->CTRL;
	
	ctrl &= ~SysTick_CLKSource_MASK;
	ctrl |= SysTick_CLKSource;
	
	SysTick->CTRL = ctrl;
}

/***************************************************************************//**
 * @brief
 *   Configure the SysTick for OS tick.
 *
 * @details
 *
 * @note
 *
 ******************************************************************************/
static void  SysTick_Configuration(void)
{

	uint32_t 	coreClk;
	uint32_t 	cnts;	

	coreClk = CMU_ClockFreqGet(cmuClock_CORE);
	cnts = coreClk / TICK_PER_SECOND;
	
	SysTick_Config(cnts);
	SysTick_CLKSourceConfig(SysTick_CLKSource_HFCORECLK);

}

void SysTick_Handler(void)
{
    	count++;
    	
    	if(count % CLOCK_CONF_SECOND == 0) 
	{
		++seconds;
		if((seconds%2) ==0)
			BSP_LedToggle(1);
		energest_flush();
	}
	
	if(etimer_pending() && (etimer_next_expiration_time() - count - 1) > MAX_TICKS) 
  	
  	{
    		etimer_request_poll();    		
#ifdef NOTYET    		
    		LPM4_EXIT;
#endif    		
  	}  
  	
  	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
  	
}

/***************************************************************************//**
 * @brief
 *   Enable SWO.
 *
 * @details
 *
 * @note
 *
 ******************************************************************************/
#if defined(EFM32_SWO_ENABLE)
void Swo_Configuration(void)
{
	uint32_t *dwt_ctrl = (uint32_t *) 0xE0001000;
	uint32_t *tpiu_prescaler = (uint32_t *) 0xE0040010;
	uint32_t *tpiu_protocol = (uint32_t *) 0xE00400F0;

    	CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;
    	/* Enable Serial wire output pin */
    	GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;
#if defined(_EFM32_GIANT_FAMILY)
    	/* Set location 0 */
    	GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

	/* Enable output on pin - GPIO Port F, Pin 2 */
	GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
	GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
	/* Set location 1 */
	GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC1;
	/* Enable output on pin */
	GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
	GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif
	/* Enable debug clock AUXHFRCO */
	CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;
	
	while(!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));
	
	/* Enable trace in core debug */
	CoreDebug->DHCSR |= 1;
	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	
	/* Enable PC and IRQ sampling output */
	*dwt_ctrl = 0x400113FF;
	/* Set TPIU prescaler to 16. */
	*tpiu_prescaler = 0xf;
	/* Set protocol to NRZ */
	*tpiu_protocol = 2;
	/* Unlock ITM and output data */
	ITM->LAR = 0xC5ACCE55;
	ITM->TCR = 0x10009;
}		
#endif
clock_time_t clock_time(void)
{
	return count;
}


int clock_fine_max(void)
{
  	return INTERVAL;
}

unsigned short
clock_fine(void)
{
  	unsigned short t;
  	/* Assign last_tar to local varible that can not be changed by interrupt */
  	t = last_tar;
  	/* perform calc based on t, TAR will not be changed during interrupt */
  	return (unsigned short) (TIMER0->CNT - t);
}

void clock_init(void)
{

#if defined(EFM32_USING_HFXO)
        /* Configure external oscillator */
        SystemHFXOClockSet(EFM32_HFXO_FREQUENCY);

        /* Switching the CPU clock source to HFXO */
        CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);

        /* Turning off the high frequency RC Oscillator (HFRCO) */
        CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);
#endif

    CMU_ClockSelectSet(cmuClock_LFA,cmuSelect_LFXO);
    CMU_ClockSelectSet(cmuClock_LFB, cmuSelect_LFXO);

#if defined(EFM32_SWO_ENABLE)
        /* Enable SWO */
        Swo_Configuration();
#endif


        /* Enable high frequency peripheral clock */
        CMU_ClockEnable(cmuClock_HFPER, true);
        /* Enabling clock to the interface of the low energy modules */
        CMU_ClockEnable(cmuClock_CORELE, true);
        /* Enable clock for TIMER0 module */
  	CMU_ClockEnable(cmuClock_TIMER0, true);
        /* Enable GPIO clock */
        CMU_ClockEnable(cmuClock_GPIO, true);

        /* Configure the SysTick */
        SysTick_Configuration();

	
}	


/*********************************************************************************************
Clock Libery function
*********************************************************************************************/

/**
 * Delay the CPU for a multiple of 2.83 us.
 */
//void clock_delay(uint32_t dlyTicks) 
void clock_delay(unsigned int dlyTicks)
{
 	uint32_t curTicks;

  	curTicks = count;
  	while ((count - curTicks) < dlyTicks) ;

}
void __delay_cycles(unsigned long c)
{
	unsigned long i;
	c/=4;
	for(i=0;i<c;i++)
	{}
}
/**
 * Wait for a multiple of 10 ms.
 *
 */
void clock_wait(clock_time_t i)
{
	clock_time_t start;

  	start = clock_time();
  	while(clock_time() - start < (clock_time_t)i);	
}

void clock_set_seconds(unsigned long sec)
{
}

unsigned long clock_seconds(void)
{
	return seconds;
}

rtimer_clock_t clock_counter(void)
{
}

