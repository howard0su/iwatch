/**************************************************************************//**
 * @file
 * @brief SPI LCD init, control and T x for EFM32GG395
 * @author Energy Micro AS
 * @version 3.20.3
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2012 Energy Micro AS, http://www.energymicro.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. The source and compiled code may only be used on Energy Micro "EFM32"
 *    microcontrollers and "EFR4" radios.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Energy Micro AS has no
 * obligation to support this Software. Energy Micro AS is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Energy Micro AS will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: SPI_LDD.c,v 0.1 2014/02/07 11:26:38 Eric Fan $
 *****************************************************************************/
#include "contiki.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include <string.h>
#include <stdio.h>
#include "platform-conf.h"

#if defined( USART_INPUT_RXPRS ) && defined( USART_TRIGCTRL_AUTOTXTEN )
#warning "USART INPUT RXPRS and TRIGCTL AUTOTXTEX"
#else
#warning "NO USART INPUT RXPRS and TRIGCTL AUTOTXTEX"
#endif

//#include "power.h"

#define MLCD_WR 0x01					// MLCD write line command
#define MLCD_CM 0x04					// MLCD clear memory command
#define MLCD_SM 0x00					// MLCD static mode command
#define MLCD_VCOM 0x02					// MLCD VCOM bit

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#define LCD_OUT		3
#define LCD_IN		4
#define	LCD_SCLK	5
#define LCD_SCS		6
/*
#define SPIDMA
*/
#ifdef W002
  #define FRAME_BUFFER_WIDTH 	160
  #define FRAME_BUFFER_STRIDE  	160
  #define FRAME_LINEBYTES	10
#else
  #define FRAME_BUFFER_WIDTH 	144
  #define FRAME_BUFFER_STRIDE 	144
  #define FRAME_LINEBYTES	9
#endif
extern void clock_delay(unsigned int dlyTicks);

static process_event_t refresh_event, clear_event;

static const uint8_t clear_cmd[2] = {MLCD_CM, 0};
static const uint8_t static_cmd[2] = {MLCD_SM, 0};
void LCDTxTransferComplete(unsigned int channel, bool primary, void *user);
void setup_SPI_DMA(void);
void memlcd_Clear(void);
//unsigned char ucLCDTxBuffer[SPITXSAMPLES] = {0x07,0x1f,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0x7f};

/* DMA structures */
DMA_CB_TypeDef dmaCallback;

static struct RefreshData
{
  	uint8_t start, end;
}data;

static enum {STATE_NONE, STATE_SENDING}state = STATE_NONE;

PROCESS(lcd_process, "LCD");

static void SPIInit()
{

  	USART_InitSync_TypeDef usartInit = USART_INITSYNC_DEFAULT;
   	TIMER_Init_TypeDef timerInit     = TIMER_INIT_DEFAULT;

   	/* Setup clocks */
   	CMU_ClockEnable( cmuClock_GPIO, true );
   	CMU_ClockEnable( cmuClock_USART2, true );

	/* Pin PB3 is configure to Push-pull as LCD_TX. To avoid false start, configure output as high  */
  	GPIO_PinModeSet(gpioPortB, LCD_OUT, gpioModePushPull, 0);
  	/* Pin PB5 is configured to Input enabled as LCD_SCLK */
  	GPIO_PinModeSet(gpioPortB, LCD_SCLK, gpioModePushPull, 0);
  	/* Pin PB6 is configure to Push-pull as LCD_SCS. To avoid false start, configure output as low(IN LCD, LCD_SCSis high chip-select)  */
  	GPIO_PinModeSet(gpioPortB, LCD_SCS, gpioModePushPull, 0);

  	/* Setup USART */
    	USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

    	init.baudrate     = 1000000;
//    	init.databits     = usartDatabits8;
    	init.databits     = usartDatabits16;
    	init.msbf         = 0;
    	init.master       = 1;
    	init.clockMode    = usartClockMode0;
    	init.prsRxEnable  = 0;
    	init.autoTx       = 0;

    	USART_InitSync(USART2, &init);
    	USART2->ROUTE = (USART_ROUTE_CLKPEN | USART_ROUTE_TXPEN | USART_ROUTE_LOCATION_LOC1);
}

void LCDTxTransferComplete(unsigned int channel, bool primary, void *user)
{
	/*Set LCD SPI CS be low (unselected)*/
	printf("lcd tx done\n");
	GPIO_PortOutClear(gpioPortB, LCD_SCS);

  	state = STATE_NONE;
  	if (data.start != 0xff)
  	{
    		process_poll(&lcd_process);

  	}
}

void setup_SPI_DMA(void)
{

	DMA_CfgChannel_TypeDef   channelConfig;
	DMA_CfgDescr_TypeDef     descriptorConfig;

		/* Setting callback function */
	dmaCallback.cbFunc = LCDTxTransferComplete;
	dmaCallback.userPtr = NULL;

	/* Setting up channel */
	channelConfig.highPri   = false;                /* No high priority */
	channelConfig.enableInt = true;                 /* Enable interrupt */

	channelConfig.select = DMAREQ_USART2_TXBL;

	channelConfig.cb        = &dmaCallback;         /* Callback routine */
#ifdef SPIDMA
  	DMA_CfgChannel(DMA_CHN_LCD_TX, &channelConfig);

  	/* Configure descriptor */
  	descriptorConfig.dstInc   = dmaDataIncNone;     /* Do not increase destination */
  	descriptorConfig.srcInc   = dmaDataInc2;        /* Increase source by 2 bytes */
  	descriptorConfig.size     = dmaDataSize2;       /* Element size is 2 bytes */
  	descriptorConfig.arbRate  = dmaArbitrate1;      /* Arbiratrate after each transfer */
  	descriptorConfig.hprot    = 0;                  /* Non-privileged access */

  	/* Configure the LOOP0 register for 2D copy */
  	DMA_CfgLoop_TypeDef loopConfig;
  	loopConfig.enable = false;
  	loopConfig.nMinus1 = FRAME_BUFFER_WIDTH/16-1;  /* Number of elements (-1) to transfer */
  	DMA_CfgLoop(DMA_CHN_LCD_TX, &loopConfig);

  	/* Configure the RECT0 register for 2D copy */
  	DMA_CfgRect_TypeDef rectConfig;
  	rectConfig.dstStride = 0;
  	rectConfig.srcStride = FRAME_BUFFER_STRIDE / 8; /* Width of the total frame buffer, in bytes */
#ifdef W002
    	rectConfig.height = 168;
#else
	rectConfig.height = 128;
#endif
  	DMA_CfgRect(DMA_CHN_LCD_TX, &rectConfig);

  	/* Create the descriptor */
  	DMA_CfgDescr(DMA_CHN_LCD_TX, true, &descriptorConfig);
#else
	DMA_CfgChannel(DMA_CHN_LCD_TX, &channelConfig);

	/* Configure descriptor */
	descriptorConfig.dstInc   = dmaDataIncNone;     /* Do not increase destination */
	descriptorConfig.srcInc   = dmaDataInc1;        /* Increase source by 2 bytes */
	descriptorConfig.size     = dmaDataSize1;       /* Element size is 2 bytes */
	descriptorConfig.arbRate  = dmaArbitrate1;      /* Arbiratrate after each transfer */
	descriptorConfig.hprot    = 0;                  /* Non-privileged access */

	/* Create the descriptor */
	DMA_CfgDescr(DMA_CHN_LCD_TX, true, &descriptorConfig);
#endif
}

#ifdef SPIDMA
static void SPISend(const void* d, unsigned int linenums)
{
	/* Enable chip select */
  	GPIO_PinOutSet( gpioPortB, LCD_SCS);

  	DMA->RECT0 = (DMA->RECT0 & ~_DMA_RECT0_HEIGHT_MASK) | linenums;

  	/* Start the transfer */
  	DMA_ActivateBasic(DMA_CHN_LCD_TX,
                    	true,                               	/* Use primary channel */
                    	false,                              	/* No burst */
                    	(void *)&(USART2->TXDOUBLE),   		/* Write to USART */
                    	(void *)(d),                       	/* Start address */
                    	FRAME_BUFFER_WIDTH/16);           	/* Width -1 */

  	state = STATE_NONE;
  	if (data.start != 0xff)
  	{
      		process_poll(&lcd_process);
  	}
}
#else
static void SPISend(const void* d, unsigned int linenums)
{
	/* Enable chip select */
  GPIO_PinOutSet( gpioPortB, LCD_SCS);

	uint16_t* cmd = (uint16_t*)d;
  uint16_t* end = (uint16_t*)((char*)d + (linenums + 1) * sizeof(linebuf) + 2);
	for(;cmd < end; cmd++)
	{
		USART_TxDouble( USART2, *cmd );
		/* Wait for transfer to finish */
		while ( !(USART2->STATUS & USART_STATUS_TXC) );
	}

  	/* SCS hold time: min 2us */
  	clock_delay(1);
  	/* Clear SCS */
  	GPIO_PinOutClear( gpioPortB, LCD_SCS );

	state = STATE_NONE;
	if (data.start != 0xff)
	{
    		process_poll(&lcd_process);
	}
}
#endif

// Initializes the display driver.
// This function initializes the LCD controller
//
// TemplateDisplayFix
void memlcd_DriverInit(void)
{
	unsigned int i;
  printf("LCD: Initialize...\n");

  memlcd_InitScreen();

	SPIInit();

	/*SET LCD_EXTCOMIN pin*/
	GPIO_PinModeSet(gpioPortC,4,gpioModePushPull,0);

  /* enable disply*/
  GPIO_PinModeSet(gpioPortA,4,gpioModePushPull,1);	// GPIOA_4 be DISP, set 1 to active display

  data.start = 0xff;
  data.end = 0;
  memlcd_Clear();
  process_start(&lcd_process, NULL);
  printf("Done\n");
}

void memlcd_DriverShutdown(void)
{
	GPIO_PinOutClear(gpioPortA, 4);	// set 1 to active display
}

void memlcd_Clear(void)
{
   	uint8_t cmd;

   	/* Set SCS */
   	GPIO_PinOutSet( gpioPortB, LCD_SCS );

   	/* SCS setup time: min 6us */
   	clock_delay(3);

   	/* Send command */
#if 1
   	cmd = MLCD_CM;
#else
   	cmd = (MLCD_CM | comPolarity);
#endif
   	USART_TxDouble( USART2, cmd );

   	/* Wait for transfer to finish */
   	while ( !(USART2->STATUS & USART_STATUS_TXC) );

   	/* SCS hold time: min 2us */
   	clock_delay(1);

   	/* Clear SCS */
   	GPIO_PinOutClear( gpioPortB, LCD_SCS );

}

void halLcdRefresh(int start, int end)
{
	dint();
	if (data.start > start)
    		data.start = start;

    	eint();
    	if (data.end < end)
    		data.end = end;

}

//*****************************************************************************
//
//! Flushes any cached drawing operations.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This functions flushes any cached drawing operations to the display.  This
//! is useful when a local frame buffer is used for drawing operations, and the
//! flush would copy the local frame buffer to the display.
//!
//! \return None.
//
//*****************************************************************************
void Template_DriverFlush(void *pvDisplayData)
{
  	if (state != STATE_NONE)
    		return;
  	dint();

  	if (data.start != 0xff)
  	{
  		eint();

    		process_post_synch(&lcd_process, refresh_event, &data);
  	}
  	else
  	{
  		eint();

  	}
}

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

PROCESS_THREAD(lcd_process, ev, d)
{
  	PROCESS_BEGIN();

  	refresh_event = process_alloc_event();
  	clear_event = process_alloc_event();

  	while(1)
  	{
    		PROCESS_WAIT_EVENT();
    		if (ev == PROCESS_EVENT_POLL)
    		{
       			// if there is an update?
			if (data.start != 0xff)
			{
				SPISend(&lines[data.start], data.end - data.start);
				dint();
				data.start = 0xff;
				eint();
				data.end = 0;
			}
			else
			{
#ifdef UNUSED
				while(UCB0STAT & UCBUSY);
				UCB0CTL1 |= UCSWRST;
				power_unpin(MODULE_LCD);
#endif
			}
    		}
    		else if (ev == refresh_event)
    		{
			if (state == STATE_NONE)
			{
				SPISend(&lines[data.start], data.end - data.start);
				dint();
				data.start = 0xff;
				eint();
				data.end = 0;
			}
    		}
    		else if (ev == clear_event)
    		{
			if (state == STATE_NONE)
	      		{
	      			memlcd_Clear();
				dint();
				data.start = 0xff;
				eint();
				data.end = 0;
	      		}
    		}
	}

  	PROCESS_END();
}

void flushlcdsync()
{
  	SPISend(&lines[0], (LCD_X_SIZE + 1) * sizeof(linebuf) + 2);
}
