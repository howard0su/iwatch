#include "contiki.h"
#include "backlight.h"
#include "sys/ctimer.h"
#include "platform-conf.h"

static struct ctimer light_timer;
static struct ctimer motor_timer;
static uint8_t LIGHTLEVEL;
#define TOP 1000       //TOP Set PWM cycle
#define HIGHPULSE  800   
#define PWM_FREQ 10000

/**************************************************************************//**
 * @brief TIMER0_IRQHandler
 * Interrupt Service Routine TIMER0 Interrupt Line
 *****************************************************************************/
void TIMER2_IRQHandler(void)
{ 
  	uint32_t compareValue;
  
  	/* Clear flag for TIMER0 overflow interrupt */
  	TIMER_IntClear(TIMER2, TIMER_IF_OF);
  	
  	TIMER_CompareBufSet(TIMER2, 0, TIMER_CaptureGet(TIMER2, 0));
  	TIMER_CompareBufSet(TIMER2, 1, TIMER_CaptureGet(TIMER2, 1));
}

void backlight_init()
{
	/*init and setup PA9(Backlight) and PA8(Motor) to GPIO output and PWM*/
	/* Enable clock for TIMER0 module */
  	CMU_ClockEnable(cmuClock_TIMER2, true);
  	/* Set  location 4 pin (PD7) as output */
  	GPIO_PinModeSet(gpioPortA, 8, gpioModePushPull, 0);
  	GPIO_PinModeSet(gpioPortA, 9, gpioModePushPull, 0);  	

  	/*PA8 PA9 at TIMER2 Location0 CC0(PA8) and CC1(PA9)*/
  	/* Select CC channel parameters */
  	TIMER_InitCC_TypeDef timerCCInit =
  	{
		.eventCtrl  = timerEventEveryEdge,
		.edge       = timerEdgeBoth,
		.prsSel     = timerPRSSELCh0,
		.cufoa      = timerOutputActionNone,
		.cofoa      = timerOutputActionNone,
		.cmoa       = timerOutputActionToggle,
		.mode       = timerCCModePWM,
		.filter     = false,
		.prsInput   = false,
		.coist      = false,
		.outInvert  = false,
  	};
  	
  	/* Configure CC channel 0 CC0 for PA8 */		
  	TIMER_InitCC(TIMER2, 0, &timerCCInit);
	/* Configure CC channel 1 CC0 for PA9 */	
	timerCCInit.prsSel = timerPRSSELCh1;	
  	TIMER_InitCC(TIMER2, 1, &timerCCInit);  	
  	
  	/* Route CC0/CC1 to location 0 (PA8/PA9) and enable pin */ 
	TIMER2->ROUTE |= (TIMER_ROUTE_CC0PEN | TIMER_ROUTE_CC1PEN | TIMER_ROUTE_LOCATION_LOC0);
  	  	
  	 /* Set Top Value */  	   	
  	TIMER_TopSet(TIMER2, CMU_ClockFreqGet(cmuClock_HFPER)/PWM_FREQ);
  
  	/*Set CCVB = 0 to TIMER2_CC0 and TIMER2_CC1 */
  	TIMER_CompareBufSet(TIMER2, 0, 0);
  	TIMER_CompareBufSet(TIMER2, 1, 0);
	  	
	/* Select timer parameters */  
  	TIMER_Init_TypeDef timerInit =
  	{
    	.enable     = true,
    	.debugRun   = true,
    	.prescale   = timerPrescale64,
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
  	TIMER_IntEnable(TIMER2, TIMER_IF_OF);  		  	
  
	/* Enable TIMER2 interrupt vector in NVIC */
  	NVIC_EnableIRQ(TIMER2_IRQn);  
  	
  	/* Configure timer */
  	TIMER_Init(TIMER2, &timerInit);  	
	
	LIGHTLEVEL = 0;
}

void backlight_shutdown()
{
 	TIMER_CompareBufSet(TIMER2, 0, 0);	
  	TIMER_CompareBufSet(TIMER2, 1, 0);	
}

void light_stop(void *ptr)
{
	if (LIGHTLEVEL > 2)
  	{
    		LIGHTLEVEL--;
    		clock_time_t length = (clock_time_t)ptr;
    		ctimer_set(&light_timer, length, light_stop, (void*)length);
  	}
  	else
  	{	
  		TIMER_CompareBufSet(TIMER2, 1, 0);	  	
  		GPIO_PinOutClear(gpioPortA, 9);		
  	}	
}

void backlight_on(uint8_t level, clock_time_t length)
{
  	if (level > 8) level = 8;

  	if (level == 0)
  	{
    		TIMER_CompareBufSet(TIMER2, 1, 0);	
  	}
  	else
  	{
  		LIGHTLEVEL = level * 2;
  		TIMER_CompareBufSet(TIMER2, 1, ( LIGHTLEVEL * 100));	
    		if (length > 0)
      			ctimer_set(&light_timer, length, light_stop, (void*)(CLOCK_SECOND/8));
  	}
}

void motor_stop(void *ptr)
{
  	TIMER_CompareBufSet(TIMER2, 0, 0);	
  	GPIO_PinOutClear(gpioPortA, 8);		
}

void motor_on(uint8_t level, clock_time_t length)
{
  	if (level > 16) level = 16;
  	if (level == 0)
  	{
    		motor_stop(NULL);
  	}
  	else
  	{
  		TIMER_CompareBufSet(TIMER2, 0, level*200);

    		if (length > 0)
      			ctimer_set(&motor_timer, length, motor_stop, NULL);
  	}
}



