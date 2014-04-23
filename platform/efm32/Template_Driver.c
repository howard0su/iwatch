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
#include "Template_Driver.h"
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

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************
static struct _linebuf
{
  uint8_t opcode;
  uint8_t linenum;
  uint8_t pixels[LCD_X_SIZE/8];
}lines[LCD_Y_SIZE]; // give a dummy

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
static void SPISend(uint8_t op, uint16_t start , const void* d, unsigned int linenums)
{
	/* Enable chip select */
  	GPIO_PinOutSet( gpioPortB, LCD_SCS);
  	
  	DMA->RECT0 = (DMA->RECT0 & ~_DMA_RECT0_HEIGHT_MASK) | linenums;
  	
  	/* Create update command and address of first line */
  	uint16_t cmd = op | (start << 8); 
	/* Send the update command */
  	USART_TxDouble(USART2, cmd);
  	
  	/* Start the transfer */
  	DMA_ActivateBasic(DMA_CHN_LCD_TX,
                    	true,                               	/* Use primary channel */
                    	false,                              	/* No burst */
                    	(void *)&(USART2->TXDOUBLE),   		/* Write to USART */
                    	(void *)(d+2),                       	/* Start address */
                    	FRAME_BUFFER_WIDTH/16 - 1);           	/* Width -1 */    	

  	state = STATE_NONE;
  	if (data.start != 0xff)
  	{
      		process_poll(&lcd_process);
  	}                    			
}
#else
static void SPISend(uint8_t op, uint16_t start , const void* d, unsigned int linenums)
{
	/* Enable chip select */
  	GPIO_PinOutSet( gpioPortB, LCD_SCS);	

	uint16_t* cmd = (uint16_t*)d;
    	for(;cmd < (char*)d + (linenums*FRAME_LINEBYTES+2); cmd++)
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
	for(i = 0; i < LCD_Y_SIZE; i++)
	{
		lines[i].linenum = i + 1;
		lines[i].opcode = MLCD_WR;
	}

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

static void halLcdRefresh(int start, int end)
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
// All the following functions (below) for the LCD driver are required by grlib
//
//*****************************************************************************

//*****************************************************************************
//
//! Draws a pixel on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the pixel.
//! \param lY is the Y coordinate of the pixel.
//! \param ulValue is the color of the pixel.
//!
//! This function sets the given pixel to a particular color.  The coordinates
//! of the pixel are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************

// TemplateDisplayFix
static void
Template_DriverPixelDraw(void *pvDisplayData, int x, int y, unsigned int ulValue)
{
  	if (ulValue == 0)
  	{
    		// if 0
    		lines[y].pixels[x/8] &= ~(1 << (x & 0x07));
  	}
  	else
  	{
    		lines[y].pixels[x/8] |= 1 << (x & 0x07);
  	}

  	halLcdRefresh(y, y);	
}

//*****************************************************************************
//
//! Draws a horizontal sequence of pixels on the screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the first pixel.
//! \param lY is the Y coordinate of the first pixel.
//! \param lX0 is sub-pixel offset within the pixel data, which is valid for 1
//! or 4 bit per pixel formats.
//! \param lCount is the number of pixels to draw.
//! \param lBPP is the number of bits per pixel; must be 1, 4, or 8.
//! \param pucData is a pointer to the pixel data.  For 1 and 4 bit per pixel
//! formats, the most significant bit(s) represent the left-most pixel.
//! \param pucPalette is a pointer to the palette used to draw the pixels.
//!
//! This function draws a horizontal sequence of pixels on the screen, using
//! the supplied palette.  For 1 bit per pixel format, the palette contains
//! pre-translated colors; for 4 and 8 bit per pixel formats, the palette
//! contains 24-bit RGB values that must be translated before being written to
//! the display.
//!
//! \return None.
//
//*****************************************************************************
static void
Template_DriverPixelDrawMultiple(void *pvDisplayData, int lX, int lY,
                                 int lX0, int lCount, int lBPP,
                                 const unsigned char *pucData,
                                 const unsigned char *pucPalette)
{
	unsigned char *pucPtr;
    	unsigned long ulByte;

    	//
    	// Check the arguments.
    	//
    	EM_ASSERT(pucData);
    	EM_ASSERT(pucPalette);

    	pucPtr = &(lines[lY].pixels[lX/8]);
    	//
    	// Determine the bit position of the starting pixel.
    	//
    	lX = lX & 7;

    	//
    	// Determine how to interpret the pixel data based on the number of bits
    	// per pixel.
    	//
    	switch(lBPP)
    	{
        //
        // The pixel data is in 1 bit per pixel format.
        //
        case 1:
        {
        	//
            	// Loop while there are more pixels to draw.
            	//
            	while(lCount)
            	{
                	// Get the next byte of image data
                	ulByte = *pucData++;

                	// Loop through the pixels in this byte of image data
                	for(; (lX0 < 8) && lCount; lX0++, lCount--)
                	{
                    		// Draw this pixel in the appropriate color
                   		*pucPtr = ((*pucPtr & ~(1 << lX)) |
                               		  ((((unsigned long *)pucPalette)[(ulByte >> (7 - lX0)) & 1]) << lX));
                    		if(lX++ == 8)
                    		{
                        		lX = 0;
                        		pucPtr++;
                    		}
                	}

                	// Start at the beginning of the next byte of image data
                	lX0 = 0;
            	}

            	//
            	// The image data has been drawn.
            	//
            	break;
        }
#if 0
        //
        // The pixel data is in 4 bit per pixel format.
        //
        case 4:
        {
            //
            // Loop while there are more pixels to draw.  "Duff's device" is
            // used to jump into the middle of the loop if the first nibble of
            // the pixel data should not be used.  Duff's device makes use of
            // the fact that a case statement is legal anywhere within a
            // sub-block of a switch statement.  See
            // http://en.wikipedia.org/wiki/Duff's_device for detailed
            // information about Duff's device.
            //
            switch(lX0 & 1)
            {
                case 0:
                    while(lCount)
                    {
                        //
                        // Get the upper nibble of the next byte of pixel data
                        // and extract the corresponding entry from the
                        // palette.
                        //
                        ulByte = (*pucData >> 4) * 3;
                        ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                  0x00ffffff);

                        //
                        // Translate this palette entry and write it to the
                        // screen.
                        //
                        *pucPtr = ((*pucPtr & ~(1 << lX)) |
                                   (DPYCOLORTRANSLATE(ulByte) << lX));
                        if(lX-- == 0)
                        {
                            lX = 7;
                            pucPtr++;
                        }

                        //
                        // Decrement the count of pixels to draw.
                        //
                        lCount--;

                        //
                        // See if there is another pixel to draw.
                        //
                        if(lCount)
                        {
                case 1:
                            //
                            // Get the lower nibble of the next byte of pixel
                            // data and extract the corresponding entry from
                            // the palette.
                            //
                            ulByte = (*pucData++ & 15) * 3;
                            ulByte = (*(unsigned long *)(pucPalette + ulByte) &
                                      0x00ffffff);

                            //
                            // Translate this palette entry and write it to the
                            // screen.
                            //
                            *pucPtr = ((*pucPtr & ~(1 << lX)) |
                                       (DPYCOLORTRANSLATE(ulByte) << lX));
                            if(lX-- == 0)
                            {
                                lX = 7;
                                pucPtr++;
                            }

                            //
                            // Decrement the count of pixels to draw.
                            //
                            lCount--;
                        }
                    }
            }

            //
            // The image data has been drawn.
            //
            break;
        }

        //
        // The pixel data is in 8 bit per pixel format.
        //
        case 8:
        {
            //
            // Loop while there are more pixels to draw.
            //
            while(lCount--)
            {
                //
                // Get the next byte of pixel data and extract the
                // corresponding entry from the palette.
                //
                ulByte = *pucData++ * 3;
                ulByte = *(unsigned long *)(pucPalette + ulByte) & 0x00ffffff;

                //
                // Translate this palette entry and write it to the screen.
                //
                *pucPtr = ((*pucPtr & ~(1 << lX)) |
                           (DPYCOLORTRANSLATE(ulByte) << lX));
                if(lX-- == 0)
                {
                    lX = 7;
                    pucPtr++;
                }
            }

            //
            // The image data has been drawn.
            //
            break;
        }
#endif
    }
    halLcdRefresh(lY, lY);
}

//*****************************************************************************
//
//! Draws a horizontal line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX1 is the X coordinate of the start of the line.
//! \param lX2 is the X coordinate of the end of the line.
//! \param lY is the Y coordinate of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a horizontal line on the display.  The coordinates of
//! the line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
Template_DriverLineDrawH(void *pvDisplayData, int lX1, int lX2,
                                   int lY, unsigned int ulValue)
{
	uint8_t *pucData = lines[lY].pixels;
	uint8_t lMask;
	
	if (ulValue) 
		ulValue = 0xffff; // 16 bit value

  	pucData += lX1 / 8;

  	// see if current buffer byte need retain
  	if (lX1 & 7)
  	{
    		lMask = 8 - (lX1 & 7);
		if (lMask > (lX2 - lX1 + 1))
		{
			lMask = lX2 - lX1 + 1;
		}
		lMask = ((1 << lMask) - 1) << ((lX1 & 7));

    		// draw the pixel
    		*pucData = (*pucData & ~lMask) | (ulValue & lMask);
    		pucData++;
    		lX1 = (lX1 + 7) & ~7;
  	}

 	if (((unsigned int)pucData & 1) && ((lX2 - lX1) > 8))
 	{
   		*pucData++ = ulValue & 0xff;
   		lX1 += 8;
 	}

 	while((lX1 + 15) <= lX2)
 	{
   		*(unsigned int *)pucData = ulValue;
   		pucData += 2;
   		lX1 += 16;
 	}

 	while((lX1 + 7) <= lX2)
 	{
   		*pucData = ulValue & 0xff;
   		pucData ++;
   		lX1 += 8;
 	}

 	if (lX1 <= lX2)
 	{
   		lMask = 0xff >> (7 - (lX2 - lX1));
   		*pucData = (*pucData & ~lMask) | (ulValue & lMask);
 	}

 	halLcdRefresh(lY, lY);
}

//*****************************************************************************
//
//! Draws a vertical line.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param lX is the X coordinate of the line.
//! \param lY1 is the Y coordinate of the start of the line.
//! \param lY2 is the Y coordinate of the end of the line.
//! \param ulValue is the color of the line.
//!
//! This function draws a vertical line on the display.  The coordinates of the
//! line are assumed to be within the extents of the display.
//!
//! \return None.
//
//*****************************************************************************
static void
Template_DriverLineDrawV(void *pvDisplayData, int lX, int lY1, int lY2,
                         unsigned int ulValue)
{
	unsigned char *pucData;
    	uint8_t lXMask;

	//
	// Determine how much to shift to get to the bit that contains this pixel.
	//
	lXMask = lX & 7;
	
	//
	// Shift the pixel value up to the correct bit position, and create a mask
	// to preserve the value of the remaining pixels.
	//
	ulValue <<= lXMask;
	lXMask = ~(1 << lXMask);
	
	//
	// Loop over the rows of the line.
	//
    	for(; lY1 <= lY2; lY1++)
    	{
		//
		// Draw this pixel of the line.
		//
		pucData = &(lines[lY1].pixels[lX/8]);
		*pucData = (*pucData & lXMask) | ulValue;
    	}

    	halLcdRefresh(lY1, lY2);
}

//*****************************************************************************
//
//! Fills a rectangle.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param pRect is a pointer to the structure describing the rectangle.
//! \param ulValue is the color of the rectangle.
//!
//! This function fills a rectangle on the display.  The coordinates of the
//! rectangle are assumed to be within the extents of the display, and the
//! rectangle specification is fully inclusive (in other words, both sXMin and
//! sXMax are drawn, along with sYMin and sYMax).
//!
//! \return None.
//
//*****************************************************************************
static void
Template_DriverRectFill(void *pvDisplayData, const tRectangle *pRect,
                                  unsigned int ulValue)
{
	int x0 = pRect->sXMin;
	int x1 = pRect->sXMax;
	int y0 = pRect->sYMin;
	int y1 = pRect->sYMax;

  	while(y0 <= y1)
  	{
    		Template_DriverLineDrawH(pvDisplayData, x0, x1, y0, ulValue);
    		y0++;
  	}
}

//*****************************************************************************
//
//! Translates a 24-bit RGB color to a display driver-specific color.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//! \param ulValue is the 24-bit RGB color.  The least-significant byte is the
//! blue channel, the next byte is the green channel, and the third byte is the
//! red channel.
//!
//! This function translates a 24-bit RGB color into a value that can be
//! written into the display's frame buffer in order to reproduce that color,
//! or the closest possible approximation of that color.
//!
//! \return Returns the display-driver specific color.
//
//*****************************************************************************
static unsigned long
Template_DriverColorTranslate(void *pvDisplayData,
                              unsigned long ulValue)
{
  	switch(ulValue)
  	{
  	case 0: 
  		return 0;
  	case 1: 
  		return 1;
  	case 2: 
  		return 2;
  	default:
    		return 1;
  	}
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
static void
Template_DriverFlush(void *pvDisplayData)
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
//! The display structure that describes the driver for the blank template.
//
//*****************************************************************************
const tDisplay g_memlcd_Driver =
{
    	sizeof(tDisplay),
    	NULL,
#if defined(PORTRAIT) || defined(PORTRAIT_FLIP)
    	LCD_Y_SIZE,
    	LCD_X_SIZE,
#else
    	LCD_X_SIZE,
    	LCD_Y_SIZE,
#endif
    	Template_DriverPixelDraw,
    	Template_DriverPixelDrawMultiple,
    	Template_DriverLineDrawH,
    	Template_DriverLineDrawV,
    	Template_DriverRectFill,
    	Template_DriverColorTranslate,
    	Template_DriverFlush
};

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
				SPISend(MLCD_WR, lines[data.start].linenum, &lines[data.start], data.end - data.start);				
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
				SPISend(MLCD_WR, lines[data.start].linenum, &lines[data.start], data.end - data.start);				
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
#ifdef W002	
	SPISend(MLCD_WR, lines[0].linenum, &lines[0], 148);
#else
	SPISend(MLCD_WR, lines[0].linenum, &lines[0], 108);
#endif	
#ifdef UNUSED
  	SPISend(&lines[0], (148 + 1) * sizeof(struct _linebuf) + 2);
#endif  	
}

#if 1
void testLCD()
{
	int i,j;
	static tContext context;	
	memlcd_DriverInit();
	memlcd_Clear();
	for(i = 0; i < LCD_Y_SIZE; i++)
	{
#ifdef W002
		for(j=0;j<18;j++)
#else
		for(j=0;j<16;j++)
#endif		
			lines[i].pixels[j] = 0xf0;
	}
	
	SPISend(MLCD_WR, lines[20].linenum, &lines[20], 50);
#ifdef UNUSED	
  	GrContextInit(&context, &g_memlcd_Driver); 	  	
	//pContext = &context;
	GrContextForegroundSet(&context, ClrWhite);
  	GrContextBackgroundSet(&context, ClrBlack);
  	const tRectangle rect = {0, 27, 255, 41};
  	GrRectFill(&context, &rect);
	
	GrFlush(&context);
#endif	
}
#endif