#include "contiki.h"
#include "button.h"
#include "window.h"
#include "system.h"
#include "sys/clock.h"
#include "sys/etimer.h"
#include "contiki-conf.h"
#include <stdint.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

PROCESS(button_process, "Button Driver");

static uint32_t downtime[4] = {0,0,0,0};
static uint32_t eventtime[4]={0,0,0,0};
struct etimer button_timer;



static const GPIOMapping ButtonArray[ MAX_BUTTONS ] = W002_GPIO_BUTTONARRAY_INIT;

static inline uint16_t getmask(int i)
{
	if (i == 0)
    		return 1 << 3;
  	else
    		return 1 << (i+5);
}

void button_init()
{
  	int i;
  	process_start(&button_process, NULL);	
  	
  	etimer_stop(&button_timer);
  	for(i=0;i<MAX_BUTTONS;i++)
  	{
  		/* Configure button GPIO as input and configure output as high */
  		GPIO_PinModeSet(ButtonArray[i].port, ButtonArray[i].pin, gpioModeInput, 1);
  		/* Set falling edge interrupt for both ports */
		GPIO_IntConfig(ButtonArray[i].port, ButtonArray[i].pin, false, true, true);
  	
  		downtime[i] = 0; 	
  	}			
  	/* Enable GPIO_ODD interrupt vector in NVIC */
  	NVIC_EnableIRQ(GPIO_ODD_IRQn);
  	NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}

int button_snapshot()
{
  	int ret;
  	ret = 0;
  	for(int i = 0; i <3; i++)
  	{
    		if (!GPIO_PinInGet(ButtonArray[i].port, ButtonArray[i].pin) )
      			ret |= (1 << i);
  	}
  	return ret;	
}

PROCESS_THREAD(button_process, ev, data)
{
	
  	PROCESS_BEGIN();
  	while(1)
  	{
    		PROCESS_WAIT_EVENT();
    		if (ev == PROCESS_EVENT_POLL)
    		{    			
      			for(int i = 0; i < 4; i++)
      			{
        			uint16_t mask = getmask(i);
       				
        			{
          				// need handle this button        				

          				if( (GPIO_PinInGet(ButtonArray[i].port, ButtonArray[i].pin) ==0) && ((GPIO->EXTIFALL&mask)!= 0) )
          				{	/*Check the GPIO pin of the button is low and int trigger is triggered by falling edge.
            					 key is down */
            					
            					//process_post(ui_process, EVENT_KEY_DOWN, (void*)i);
            					downtime[i] = eventtime[i]; 
            					printf("key[%d] press \n", i);            					
            					if (etimer_expired(&button_timer))
            					{
              						// first button
              						etimer_set(&button_timer, CLOCK_SECOND/2);
            					}
            					/* Reverse the trigger edge(Rising or Falling) */
            					/* EFM32 do not need it , EFM32 can set Rising edge trigger and Falling trigger at the same time. */            					
            					GPIO_IntConfig(ButtonArray[i].port, ButtonArray[i].pin, true, false, true);
          				}          				
          				else if( (GPIO_PinInGet(ButtonArray[i].port, ButtonArray[i].pin) ==1) && ((GPIO->EXTIRISE&mask)!=0) )
          				{	
          					/*Check the GPIO pin of the button is high and int trigger is triggered by rising edge.*/
          					printf("key [%d] release \n", i);

            					//process_post(ui_process, EVENT_KEY_UP, (void*)i);
            					if (downtime[i] > 0)
            					{
            						if (eventtime[i] - downtime[i] > 3000)              					
              							printf("button long press\n");
#ifdef NOTYET              							
                						process_post(ui_process, EVENT_KEY_LONGPRESSED, (void*)i);
#endif                						
              						else
              							printf("button short press\n");
#ifdef NOTYET      
                						process_post(ui_process, EVENT_KEY_PRESSED, (void*)i);
#endif                						
            					}
            					downtime[i] = 0;            					
            					GPIO_IntConfig(ButtonArray[i].port, ButtonArray[i].pin, false, true, true);
          				}          				
          				else if( (GPIO_PinInGet(ButtonArray[i].port, ButtonArray[i].pin) ==0) && ((GPIO->EXTIRISE&mask)!=0) )
          				{	/*Check the GPIO pin of the button is low and int trigger is triggered by rising edge.*/         
          					printf("key hold press\n"); 					
            					if (etimer_expired(&button_timer))
            					{
              						// first button
              						etimer_set(&button_timer, CLOCK_SECOND/2);
            					}
          				}          				
          				else if( (GPIO_PinInGet(ButtonArray[i].port, ButtonArray[i].pin) ==1) && ((GPIO->EXTIFALL&mask)!=0) )
          				{	/*Check the GPIO pin of the button is high and int trigger is triggered by falling edge.*/
            					downtime[i] = 0;
          				}
          				
          				GPIO_IntEnable(mask);
        			}
      			}

    		}
    		else if (ev == PROCESS_EVENT_TIMER)
    		{
      			uint8_t reboot = 0;
			uint8_t downbutton = 0;
			uint8_t needquick = 0;
      			for(int i = 0; i < 4; i++)
      			{
        			PRINTF("button %d downtime: %ld\n", i, downtime[i]);
        			if (downtime[i] > 0)
        			{
          				downbutton++;
          				if (downtime[i] < clock_time() - RTIMER_SECOND * 3)
            					reboot++;
					if (downtime[i] < clock_time() - RTIMER_SECOND)
          				{
            					// check if we need fire another event
            					printf("button PRESS\n");
#ifdef NOTYET
            					process_post(ui_process, EVENT_KEY_PRESSED, (void*)i);
#endif
            					needquick++;
          				}
        			}
      			}
			PRINTF("%d buttons down, now=%ld\n", downbutton, clock_time());

      			if (reboot == 4)
        			system_reset();

      			if (downbutton)
      			{
        			if (needquick)
          				etimer_set(&button_timer, CLOCK_SECOND/4);
        			else
          				etimer_set(&button_timer, CLOCK_SECOND/2);
      			}
      			else
      			{
          			etimer_stop(&button_timer);
      			}

    		}
  	}	
  	PROCESS_END();
}

static inline int portF_button(int i)
{
	GPIO_IntDisable(getmask(i));	//disable pin_i interrupt
	GPIO_IntClear(getmask(i));	//Clear interrupt flag
  	eventtime[i] = clock_time();
  	process_poll(&button_process);
  	return 1;
}

int portF_pin3()
{
	return portF_button(0);
}

int portF_pin6()
{
	return portF_button(1);
}

int portF_pin7()
{
	return portF_button(2);
}

int portF_pin8()
{
	return portF_button(3);
}