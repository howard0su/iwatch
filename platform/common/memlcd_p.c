#include "contiki.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include <string.h>
#include <stdio.h>
 
#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

//*****************************************************************************
//
// Global Variables
//
//*****************************************************************************
linebuf lines[LCD_Y_SIZE];

extern void halLcdRefresh(int start, int end);

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
Template_DriverPixelDraw(void *pvDisplayData, int _x, int _y,
                                   unsigned int ulValue)
{
  int x = MAPPED_X(_x, _y);
  int y = MAPPED_Y(_x, _y);

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
Template_DriverLineDrawH(void *pvDisplayData, int _lX1, int _lX2,
                                   int _lY, unsigned int ulValue)
{
  #if 1
  int lY1 = MAPPED_Y(_lX1, _lY);
  int lY2 = MAPPED_Y(_lX2, _lY);
  int lX = MAPPED_X(_lX1, _lY);

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
#endif
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
Template_DriverLineDrawV(void *pvDisplayData, int _lX, int _lY1, int _lY2,
                         unsigned int ulValue)
{
  #if 1
  int lX1 = MAPPED_Y(_lX, _lY1);
  int lX2 = MAPPED_Y(_lX, _lY2);
  int lY = MAPPED_X(_lX, _lY1);

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

 if (((uint8_t)pucData & 1) && ((lX2 - lX1) > 8))
 {
   *pucData++ = ulValue & 0xff;
   lX1 += 8;
 }

 while((lX1 + 15) <= lX2)
 {
   *(uint16_t *)pucData = ulValue;
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
#endif
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
  case 0: return 0;
  case 1: return 1;
  case 2: return 2;
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
extern void
Template_DriverFlush(void *pvDisplayData);

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

void memlcd_InitScreen(void)
{
  unsigned int i;
  for(i = 0; i < LCD_Y_SIZE; i++)
  {
    lines[i].linenum = i + 1;
    lines[i].opcode = MLCD_WR;
  }
}