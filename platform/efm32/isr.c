#include "contiki.h"
//#include "isr_compat.h"

extern int portF_pin3();
extern int portF_pin6();
extern int portF_pin7();
extern int portF_pin8();
//For BT RTS interrupt
extern int portB_pin11();		
void GPIO_ODD_IRQHandler(void)
{	
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
	uint32_t intX = GPIO_IntGet();
	if(intX & 0x0008)
	{	
		/*GPIOx_pin3 int : button1 */
		GPIO_IntClear(0x0008);
		if (portF_pin3())
		{			
#ifdef NOTYET			
        		LPM4_EXIT;										
#endif        		
		}	
	}	
		
	if(intX & 0x80)
	{	
		/*GPIOx_pin7 int : button3*/
		GPIO_IntClear(0x0080);
		if (portF_pin7())
		{
#ifdef NOTYET			
        		LPM4_EXIT;		
#endif								
		}	
	}	

	if(intX & 0x0800)
	{	
		/*GPIOx_pin11 INT_MAG : LSM303C INT_MAG*/
		/*For BT RTS interrupt*/
		GPIO_IntClear(0x0800);
#ifdef NOTYET
		if(portB_pin11())
		{
			
		}	
#endif		
		
		
		
	}	

	if(intX & 0x2000)
	{	
		/*GPIOx_pin13 INT_XL_MAG/INT2 : LSM303C INT_XL_MAG*/
		GPIO_IntClear(0x2000);
		
	}		

	
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}

void GPIO_EVEN_IRQHandler(void)
{
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
	uint32_t intX = GPIO_IntGet();

	if(intX & 0x0010)
	{
		/*GPIOx_pin4 GA_INT : MPU3050 int*/
		GPIO_IntClear(0x0010);
	}		
	
	if(intX & 0x0040)
	{	
		/*GPIOx_pin6 int : button2*/
		GPIO_IntClear(0x0040);
      		if (portF_pin6())
      		{	
      			printf("$PIOF 6\n");
#ifdef NOTYET			
        		LPM4_EXIT;		
#endif								      			

		}
	}		
	if(intX & 0x0100)
	{	
		/*GPIOx_pin8 int : button4*/
		GPIO_IntClear(0x0100);
      		if (portF_pin8())
		{      			
			printf("$PIOF 8\n");
#ifdef NOTYET			
        		LPM4_EXIT;		
#endif								
		}	
	}			

	if(intX & 0x1000)
	{	
		/*GPIOx_pin12 DRDY_MAG/INT1 : LSM303C DRDY_MAG*/
		GPIO_IntClear(0x1000);
		
	}		

	ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
