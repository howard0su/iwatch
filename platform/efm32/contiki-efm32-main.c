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
#include "contiki-conf.h"
#include "dev/watchdog.h"
#include "system.h"
#include "sys/autostart.h"
#include "spiflash.h"
#include "i2c.h"
#include "window.h"
#include "backlight.h"
#include "btstack/bluetooth.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

/*
#include "dev/flash.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/watchdog.h"
#include "dev/xmem.h"
#include "lib/random.h"
#include "sys/autostart.h"
#include "sys/profile.h"
#include "sys/ctimer.h"

#include "node-id.h"
#include "power.h"
#include "battery.h"
#include "spiflash.h"
#include "btstack/bluetooth.h"
#include "backlight.h"
#include "window.h"

#include "ant/ant.h"
#include "ant/antinterface.h"
*/
/*--------------------------------------------------------------------------*/
#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//extern void mpu6050_init();
extern void mpu3050Init(void);
extern void READ_MPU3050(void);
extern void button_init();
extern void I2C_Init();
extern void rtc_init();
extern void battery_init();
extern void testLCD();
extern void efm32_cpu_init(void);
extern void clock_delay(unsigned int dlyTicks);

static uint8_t msp430_dco_required = 0;
uint8_t shutdown_mode = 0;
/*--------------------------------------------------------------------------*/

int
main(int argc, char **argv)
{	
	/*
	* Initalize hardware.
	*/
 
	/* Enable code view */	        
	/* If first word of user data page is non-zero, enable eA Profiler trace */  	
	efm32_cpu_init();  	
	clock_init();
	printf("CPU init done\n");	
	DMAInit();	
	/* xmem_init(); */
//	PRINTF("iWatch 0.10 build at " __TIME__ " " __DATE__ "\n");
	
	/*
	* Hardware initialization done!
	*/
	
	/*
	* Initialize Contiki and our processes.
	*/

	process_init();
	process_start(&etimer_process, NULL);
	
	rtimer_init();	
	ctimer_init();

	energest_init();
	ENERGEST_ON(ENERGEST_TYPE_CPU);
	
	backlight_init();
	battery_init();
	SPI_FLASH_Init();
	
	int reason = CheckUpgrade();	
	window_init(reason);	
	
	button_init();

	rtc_init();

	CFSFontWrapperLoad();

	system_init(); // check system status and do factor reset if needed

	I2C_Init_();	
	mpu3050Init();
#ifdef UNUSED
	codec_init();	
	ant_init();
#endif
	printf("Start bluetooth\n");
	bluetooth_init();

	printf("Bluetooth Started\n");	
	motor_on(6, CLOCK_SECOND );		
	
#ifdef NOTYET	
	if (!system_retail())
	{
		bluetooth_discoverable(1);
	}
#endif	
#ifdef UNUSED
	ant_init(MODE_HRM);
#endif
	//  protocol_init();
	//  protocol_start(1);
	process_start(&system_process, NULL);
	/*
	* This is the scheduler loop.
	*/	
	
  	/*
    	check firmware update
    	*/
#ifdef NOTYET	    	
  	if (CheckUpgrade() == 0)
  	{
    		printf("Start Upgrade\n");
    		Upgrade();
    		// never return if sucessfully upgrade
  	}	
#endif  	
	watchdog_start();
	
  	while(1) 
  	{
    		int r;
    		do {
	      	/* Reset watchdog. */
	      	watchdog_periodic();
	      	r = process_run();
	    	} while(r > 0);

	    	/*
	    	* Idle processing.
	    	*/
#ifdef NOTYET		    	
		int s = splhigh();          /* Disable interrupts. */
#endif		
		/* uart1_active is for avoiding LPM3 when still sending or receiving */
		if(process_nevents() != 0) 
		{
#ifdef NOTYET				
			splx(s);                  /* Re-enable interrupts. */
#endif
		} 
		else 
		{
			static unsigned long irq_energest = 0;

			/* Re-enable interrupts and go to sleep atomically. */
			ENERGEST_OFF(ENERGEST_TYPE_CPU);
			ENERGEST_ON(ENERGEST_TYPE_LPM);
			/* We only want to measure the processing done in IRQs when we
			 are asleep, so we discard the processing time done when we
			 were awake. */
			energest_type_set(ENERGEST_TYPE_IRQ, irq_energest);
			watchdog_stop();

			if (shutdown_mode)
			{
#ifdef NOTYET				
				system_shutdown(); // never return
#endif				
			}
			
			
			EMU_EnterEM1();
#ifdef NOTYET
			if (msp430_dco_required)
			{				
				EMU_EnterEM1();								
			}
			else
			{				
				EMU_EnterEM2(true);
			}
#endif
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

/*---------------------------------------------------------------------------*/
/* add/remove_lpm_req - for requiring a specific LPM mode. currently Contiki */
/* jumps to LPM3 to save power, but DMA will not work if DCO is not clocked  */
/* so some modules might need to enter their LPM requirements                */
/* NOTE: currently only works with LPM1 (e.g. DCO) requirements.             */
/*---------------------------------------------------------------------------*/

void
power_pin(uint8_t module)
{
    msp430_dco_required |= module;
}

void
power_unpin(uint8_t module)
{
    msp430_dco_required &= ~module;
}

