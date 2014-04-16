#include "contiki.h"
#include "i2c.h"
#include "platform-conf.h"
#include "sys/rtimer.h"
#include <stdio.h>

/*
static uint8_t *rxdata;
static uint16_t rxlen;
static const uint8_t *payload;
static uint8_t payloadlen;
static const uint8_t *txdata;
static uint8_t txlen;
*/
static enum { STATE_IDL, STATE_RUNNING, STATE_DONE, STATE_ERROR} state;

/*******************************************************************************
 **************************   GLOBAL VARIABLES   *******************************
 ******************************************************************************/
I2C_TransferReturn_TypeDef I2C_Status;

/*******************************************************************************
 **************************   GLOBAL FUNCTIONS   *******************************
 ******************************************************************************/

/**************************************************************************//**
 * @brief I2C Interrupt Handler.
 *        The interrupt table is in assembly startup file startup_efm32.s
 *****************************************************************************/
void I2C0_IRQHandler(void)
{
  	/* Just run the I2C_Transfer function that checks interrupts flags and returns */
  	/* the appropriate status */
  	I2C_Status = I2C_Transfer(I2C0);
}

void I2C_Init_()
{
	int i;
  	printf("I2C: Initialize...\n");
  
	/* Initialize I2C driver for the tempsensor on the DK, using standard rate. */
	/* Devices on DK itself supports fast mode, */
	/* but in case some slower devices are added on */
	/* prototype board, we use standard mode. */
	I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;  
  
  	CMU_ClockEnable(cmuClock_I2C0, true);
  	
  	/* Use location 3: SDA - Pin D14, SCL - Pin D15 */
  	/* Output value must be set to 1 to not drive lines low... We set */
  	/* SCL first, to ensure it is high before changing SDA. */
  	GPIO_PinModeSet(gpioPortD, 15, gpioModeWiredAnd, 1);
  	GPIO_PinModeSet(gpioPortD, 14, gpioModeWiredAnd, 1);  	
  	
  	/* In some situations (after a reset during an I2C transfer), the slave */
  	/* device may be left in an unknown state. Send 9 clock pulses just in case. */
  	for (i = 0; i < 9; i++)
  	{
		/*
		* TBD: Seems to be clocking at appr 80kHz-120kHz depending on compiler
		* optimization when running at 14MHz. A bit high for standard mode devices,
		* but DVK only has fast mode devices. Need however to add some time
		* measurement in order to not be dependable on frequency and code executed.
		*/
		GPIO_PinModeSet(gpioPortD, 15, gpioModeWiredAnd, 0);
		GPIO_PinModeSet(gpioPortD, 15, gpioModeWiredAnd, 1);
  	}  	
  	
  	/* Enable pins at location 3 (which is used on the DVK) */
  	I2C0->ROUTE = I2C_ROUTE_SDAPEN |
                	I2C_ROUTE_SCLPEN |
                	(3 << _I2C_ROUTE_LOCATION_SHIFT);  	
  
  	I2C_Init(I2C0, &i2cInit);
  	
  	/* Clear and enable interrupt from I2C module */
  	NVIC_ClearPendingIRQ(I2C0_IRQn);
  	NVIC_EnableIRQ(I2C0_IRQn);
 
 	printf("\n$$OK I2C\n");
}

void I2C_shutdown()
{
	I2C_Enable(I2C0, false);

}


int I2C_readbytes(I2C_TypeDef *i2c, uint8_t addr, unsigned char reg, unsigned char *data, uint16_t len)
{
	I2C_TransferSeq_TypeDef seq;
  	uint8_t regid[1];  	
  	
  	seq.addr = addr;
  	seq.flags = I2C_FLAG_WRITE_READ;
  	/* Select register to be read */
  	regid[0] = ((uint8_t)reg);
  	/*buf[0] be TX buf*/
  	seq.buf[0].data = regid;	
  	seq.buf[0].len = 1;  	
  	
  	/*buf[1] for RX buf*/
  	seq.buf[1].data = data;
    	seq.buf[1].len = len;
    	
    	/* Do a polled transfer */
  	I2C_Status = I2C_TransferInit(i2c, &seq);
  	while (I2C_Status == i2cTransferInProgress)
  	{
    		/* Enter EM1 while waiting for I2C interrupt */
    		EMU_EnterEM1();
    		/* Could do a timeout function here. */
  	}
  
  	if (I2C_Status != i2cTransferDone)
  	{
    		return((int)I2C_Status);
  	}

  	return(0); 	
}

int I2C_writebytes(I2C_TypeDef *i2c, uint8_t addr, unsigned char reg, const unsigned char *data, uint8_t len)
{
	I2C_TransferSeq_TypeDef seq;
  	uint8_t regid[1];  	
  	
  	seq.addr = addr;
  	seq.flags = I2C_FLAG_WRITE;
  	/* Select register to be write */
  	regid[0] = ((uint8_t)reg);
  	
  	seq.buf[0].data   = data;
  	seq.buf[0].len    = len;
  	
   	/* Do a polled transfer */
  	I2C_Status = I2C_TransferInit(i2c, &seq);
  	while (I2C_Status == i2cTransferInProgress)
  	{
    		/* Enter EM1 while waiting for I2C interrupt */
    		EMU_EnterEM1();
    		/* Could do a timeout function here. */
  	}
  
  	return(I2C_Status);
  	
  		
}

int I2C_write(I2C_TypeDef *i2c, uint8_t addr, unsigned char reg, const unsigned char data)
{
	I2C_TransferSeq_TypeDef seq;
  	uint8_t regid[1];  	
  	
  	seq.addr = addr;
  	seq.flags = I2C_FLAG_WRITE;
  	/* Select register to be write */
  	regid[0] = ((uint8_t)reg);
  	
  	seq.buf[0].data   = &data;
  	seq.buf[0].len    = 1;
  	
   	/* Do a polled transfer */
  	I2C_Status = I2C_TransferInit(i2c, &seq);
  	while (I2C_Status == i2cTransferInProgress)
  	{
    		/* Enter EM1 while waiting for I2C interrupt */
    		EMU_EnterEM1();
    		/* Could do a timeout function here. */
  	}
  
  	return(I2C_Status);	
}

void I2C_done()
{
	I2C_Enable(I2C0, false);  	
}
