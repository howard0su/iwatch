/* --COPYRIGHT--,BSD
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
//*****************************************************************************
//
// Template_Driver.c - Display driver for any LCD Controller. This file serves as
//						a template for creating new LCD driver files
//
// Copyright (c) 2008-2011 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
//
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
//
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
//
//*****************************************************************************
//
//! \addtogroup display_api
//! @{
//
//*****************************************************************************

//*****************************************************************************
//
// READ ME
//
// This template driver is intended to be modified for creating new LCD drivers
// It is setup so that only Template_DriverPixelDraw() and DPYCOLORTRANSLATE()
// and some LCD size configuration settings in the header file Template_Driver.h
// are REQUIRED to be written. These functions are marked with the string
// "TemplateDisplayFix" in the comments so that a search through Template_Driver.c and
// Template_Driver.h can quickly identify the necessary areas of change.
//
// Template_DriverPixelDraw() is the base function to write to the LCD
// display. Functions like WriteData(), WriteCommand(), and SetAddress()
// are suggested to be used to help implement the Template_DriverPixelDraw()
// function, but are not required. SetAddress() should be used by other pixel
// level functions to help optimize them.
//
// This is not an optimized driver however and will significantly impact
// performance. It is highly recommended to first get the prototypes working
// with the single pixel writes, and then go back and optimize the driver.
// Please see application note www.ti.com/lit/pdf/slaa548 for more information
// on how to fully optimize LCD driver files. In short, driver optimizations
// should take advantage of the auto-incrementing of the LCD controller.
// This should be utilized so that a loop of WriteData() can be used instead
// of a loop of Template_DriverPixelDraw(). The pixel draw loop contains both a
// SetAddress() + WriteData() compared to WriteData() alone. This is a big time
// saver especially for the line draws and Template_DriverPixelDrawMultiple.
// More optimization can be done by reducing function calls by writing macros,
// eliminating unnecessary instructions, and of course taking advantage of other
// features offered by the LCD controller. With so many pixels on an LCD screen
// each instruction can have a large impact on total drawing time.
//
//*****************************************************************************


//*****************************************************************************
//
// Include Files
//
//*****************************************************************************
#include "contiki.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <string.h>

#define SPIOUT  P3OUT
#define SPIDIR  P3DIR
#define SPISEL  P3SEL

#define V5VOUT  P6OUT
#define V5VDIR  P6DIR
#define V5BIT   BIT6

#define _SCLK	BIT3					// SPI clock
#define _SDATA	BIT1					// SPI data (sent to display)
#define _SCS	BIT0					// SPI chip select

#define MLCD_WR 0x01					// MLCD write line command
#define MLCD_CM 0x04					// MLCD clear memory command
#define MLCD_SM 0x00					// MLCD static mode command
#define MLCD_VCOM 0x02					// MLCD VCOM bit

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static process_event_t refresh_event, clear_event;

static const uint8_t clear_cmd[2] = {MLCD_CM, 0};
static const uint8_t static_cmd[2] = {MLCD_SM, 0};

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


// Initializes the display driver.
// This function initializes the LCD controller
//
// TemplateDisplayFix
void
memlcd_DriverInit(void)
{
  unsigned int i;
  for(i = 0; i < LCD_Y_SIZE; i++)
  {
    lines[i].linenum = i + 1;
    lines[i].opcode = MLCD_WR;
  }
}

static void halLcdRefresh(int start, int end)
{
  if (data.start > start)
    data.start = start;
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
Template_DriverPixelDraw(void *pvDisplayData, int x, int y,
                                   unsigned int ulValue)
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
Template_DriverPixelDrawMultiple(void *pvDisplayData, long lX, long lY,
                                 long lX0, long lCount, long lBPP,
                                 const unsigned char *pucData,
                                 const unsigned char *pucPalette)
{
    unsigned char *pucPtr;
    unsigned long ulByte;

    //
    // Check the arguments.
    //
    ASSERT(pucData);
    ASSERT(pucPalette);

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
                               ((((unsigned long *)pucPalette)[(ulByte >>
                                                                (7 - lX0)) &
                                                               1]) << lX));
                    if(lX++ == 8)
                    {
                        lX = 0;
                        pucPtr++;
                    }
                }

                // Start at the beginning of the next byte of image data
                lX0 = 0;
            }
            // The image data has been drawn

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

  if (ulValue) ulValue = 0xffff; // 16 bit value

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
   *(uint16_t*)pucData = ulValue;
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
Template_DriverLineDrawV(void *pvDisplayData, long lX, long lY1, long lY2,
                         unsigned long ulValue)
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
static unsigned int
Template_DriverColorTranslate(void *pvDisplayData,
                                        unsigned long ulValue)
{
  switch(ulValue)
  {
  case 0: return 0;
  case 1: return 1;
  case 2: return 2;
  default:
    return 1;
  }
}

static void screenshot(void);
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
  if (data.start != 0xff)
  {
    screenshot();
  }
}

//*****************************************************************************
//
//! Send command to clear screen.
//!
//! \param pvDisplayData is a pointer to the driver-specific data for this
//! display driver.
//!
//! This function does a clear screen and the Display Buffer contents
//! are initialized to the current background color.
//!
//! \return None.
//
//*****************************************************************************
static void
Template_DriverClearScreen (void *pvDisplayData, unsigned int ulValue)
{
  unsigned int i;

  for(i = 0; i < LCD_Y_SIZE; i++)
  {
    memset(lines[i].pixels, 0, LCD_X_SIZE/8);
  }

  halLcdRefresh(0, LCD_Y_SIZE);
  //process_post_synch(&lcd_process, clear_event, NULL);
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

static uint8_t lookup[16] = {
   0x0, 0x8, 0x4, 0xC,
   0x2, 0xA, 0x6, 0xE,
   0x1, 0x9, 0x5, 0xD,
   0x3, 0xB, 0x7, 0xF };

static uint8_t flip( uint8_t n )
{
   //This should be just as fast and it is easier to understand.
   //return (lookup[n%16] << 4) | lookup[n/16];
   return (lookup[n&0x0F] << 4) | lookup[n>>4];
}

#include <stdio.h>
void screenshot()
{
  static int id;
  FILE* fp;
  watchdog_stop();
  char name[30];
  sprintf(name, "unittest/screen/image%03d.pbm", id++);
  fp = fopen(name, "wb");
  fprintf(fp, "P4\n%d %d\n", LCD_X_SIZE, LCD_Y_SIZE);

  for(int i = 0; i < LCD_Y_SIZE; i++)
  {
    uint8_t buf[LCD_X_SIZE/8];
    for(int j = 0; j < LCD_X_SIZE/8; j++)
    {
     buf[j] = ~(flip(lines[i].pixels[j]));
    }
    fwrite(buf, LCD_X_SIZE/8, 1, fp);
  }

  fclose(fp);
  watchdog_start();
}
