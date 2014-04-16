#include "contiki.h"
#include "battery.h"

#include <stdio.h>

static uint8_t level;
uint8_t uartattached = 1;

#define MSP_OUT		7
#define MSP_IN		4

/***************************************************************************//**
* @brief
*   Configure ADC usage for this application.
*******************************************************************************/
static void ADCConfig(void)
{
  	ADC_Init_TypeDef       init       = ADC_INIT_DEFAULT;
  	ADC_InitSingle_TypeDef singleInit = ADC_INITSINGLE_DEFAULT;

  	/* Init common settings for both single conversion and scan mode */
  	init.timebase = ADC_TimebaseCalc(0);
  	/* Might as well finish conversion as quickly as possibly since polling */
  	/* for completion. */
  	/* Set ADC clock to 7 MHz, use default HFPERCLK */
  	init.prescale = ADC_PrescaleCalc(7000000, 0);

  	/* WARMUPMODE must be set to Normal according to ref manual before */
  	/* entering EM2. In this example, the warmup time is not a big problem */
  	/* due to relatively infrequent polling. Leave at default NORMAL, */

  	ADC_Init(ADC0, &init);

  	/* Init for single conversion use, measure VDD/3 with 1.25 reference. */
  	singleInit.reference  = adcRef2V5;
  	//singleInit.reference  = adcRef1V25;
  	singleInit.input      = adcSingleInpCh5;	
  	//singleInit.input      = adcSingleInpVDDDiv3;
  	singleInit.resolution = adcRes12Bit;

  	/* The datasheet specifies a minimum aquisition time when sampling vdd/3 */
  	/* 32 cycles should be safe for all ADC clock frequencies */
  	singleInit.acqTime = adcAcqTime32;

  	ADC_InitSingle(ADC0, &singleInit);
}


static void setoutputfloat()
{
  	// set high impredence for BAT_OUT
  	GPIO_PinModeSet(gpioPortD,  MSP_OUT, gpioModeDisabled, 0);  	 
 
}

static void setoutputhigh()
{
	GPIO_PinModeSet(gpioPortD,  MSP_OUT, gpioModePushPull, 1);  	 
}

void battery_init(void)
{
	GPIO_PinModeSet(gpioPortD, MSP_IN, gpioModeInput, 0);
	CMU_ClockEnable(cmuClock_ADC0, true);
	ADCConfig();	
}

BATTERY_STATE battery_state(void)
{
	return BATTERY_STATE_FULL;
}

// map battery level to 0-15 scale
const uint8_t curve[] = 
{
  183, 185, 187, 189, 190, 191, 192, 193, 194, 195, 198, 200, 204, 207, 209, 212
};


uint8_t battery_level(BATTERY_STATE state)
{
#if 0
  	uint32_t 	sample;
  	uint32_t   	level;	
  	uint8_t 	ret;
  	
  	
	ADC_Start(ADC0, adcStartSingle);
	/* Wait while conversion is active */
    	while (ADC0->STATUS & ADC_STATUS_SINGLEACT) ;

    	/* Get ADC result */
    	sample = ADC_DataSingleGet(ADC0);
    	
    	/* Calculate supply voltage relative to 1.25V reference */    	
	level = (sample * 2500 ) / 4096;    	
	
	for(ret = 0; ret< sizeof(curve); ret++)
	{
		if (level < curve[ret])
      			return ret - 1;		
	}
	
	return 15;
#else 
	return 14;
#endif	
}