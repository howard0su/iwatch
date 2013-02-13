/****************************************************************
*  Description: Implementation for SHARP memory LCD
*    History:
*      Jun Su          2013/1/2        Created
*      Jun Su          2013/1/16       Use DMA
*      Jun Su          2013/1/17       Use hardware COM switch
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include "contiki.h"
#include <string.h>

#include "sys/etimer.h"
#include "hal_lcd.h"
#include "font.h"

#define SPIOUT  P3OUT
#define SPIDIR  P3DIR
#define SPISEL  P3SEL

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

extern void doubleWideAsm(unsigned char c, unsigned char* buff);

PROCESS(lcd_process, "LCD");

static process_event_t refresh_event, clear_event;

enum {STATE_NONE, STATE_SENDING};
static unsigned char state = STATE_NONE;

struct RefreshData
{
  unsigned char start, end;
};

static struct _linebuf
{
  unsigned char opcode;
  unsigned char linenum;
  unsigned char pixels[LCD_ROW/8];
}lines[LCD_COL + 1]; // give a dummy


static void SPIInit()
{
  UCB0CTL1 = UCSWRST;

  UCB0CTL0 = UCMODE0 + UCMST + UCSYNC + UCCKPH; // master, 3-pin SPI mode, LSB
  UCB0CTL1 |= UCSSEL__SMCLK; // SMCLK for now
  UCB0BR0 = 8; // 8MHZ / 8 = 1Mhz
  UCB0BR1 = 0;

  //Configure ports.
  SPIDIR |= _SCLK | _SDATA | _SCS;
  SPISEL |= _SCLK | _SDATA;
  SPIOUT &= ~(_SCLK | _SDATA | _SCS);

  UCB0CTL1 &= ~UCSWRST;
}

static struct RefreshData data;
static unsigned char suspendUpdate;

void halLcdBeginUpdate()
{
  suspendUpdate = 1;
  data.start = 0xff;
  data.end = 0;
}

void halLcdEndUpdate()
{
  if (suspendUpdate)
  {
    suspendUpdate = 0;
    process_post_synch(&lcd_process, refresh_event, &data);
  }
}

static void halLcdRefresh(int start, int end)
{
  if (data.start > start)
    data.start = start;
  if (data.end < end)
    data.end = end;

  if (!suspendUpdate)
  {
    process_post_synch(&lcd_process, refresh_event, &data);
  }
}

void halLcdBackLightInit()
{
}

void halLcdSetBackLight(unsigned char n)
{
}

void halLcdInit()
{
  unsigned int i;
  for(i = 0; i < LCD_COL; i++)
  {
    lines[i].linenum = i;
    lines[i].opcode = MLCD_WR;
  }

  SPIInit();

  // configure TA0.1 for COM switch
  TA0CTL |= TASSEL_1 + ID_3 + MC_1;
  TA0CCTL1 = OUTMOD_7;
  TA0CCR0 = 4096;
  TA0CCR1 = 1;

  P8SEL |= BIT1;
  P8DIR |= BIT1; // p8.1 is TA0.1

  // enable disply
  P8DIR |= BIT2; // p8.2 is display
  P8OUT |= BIT2; // set 1 to active display

  process_start(&lcd_process, NULL);
}

void halLcdClearScreen()
{
  unsigned int i;

  for(i = 0; i < LCD_COL; i++)
  {
    memset(lines[i].pixels, 0xff, LCD_ROW/8);
  }

  process_post_synch(&lcd_process, clear_event, NULL);
}

static void SPISend(void* data, unsigned int size)
{
  PRINTF("Send Data %d bytes\n", size);
  state = STATE_SENDING;
  SPIOUT |= _SCS;

  // USB0 TXIFG trigger
  DMACTL0 = DMA0TSEL_19;
  // Source block address
  __data16_write_addr((unsigned short) &DMA0SA,(unsigned long) data);
  // Destination single address
  __data16_write_addr((unsigned short) &DMA0DA,(unsigned long) &UCB0TXBUF);
  DMA0SZ = size;                                // Block size
  DMA0CTL &= ~DMAIFG;
  DMA0CTL = DMASRCINCR_3+DMASBDB+DMALEVEL + DMAIE + DMAEN;  // Repeat, inc src
}

int dma_channel_0()
{
  process_poll(&lcd_process);

  return 1;
}

static unsigned char clear_cmd[2] = {MLCD_CM, 0};
//static unsigned char static_cmd[2] = {MLCD_SM, 0};


PROCESS_THREAD(lcd_process, ev, data)
{
  static unsigned char refreshStart = 0xff, refreshStop = 0;

  PROCESS_BEGIN();

  refresh_event = process_alloc_event();
  clear_event = process_alloc_event();

  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_POLL)
    {
      SPIOUT &= ~_SCS;
      state = STATE_NONE;

      if (refreshStart != 0xff)
      {
        SPISend(&lines[refreshStart], (refreshStop - refreshStart + 1)
                * sizeof(struct _linebuf) + 1);
        refreshStart = 0xff;
        refreshStop = 0;
      }
    }
    else if (ev == refresh_event)
    {
      struct RefreshData* d = (struct RefreshData*)data;
      if (refreshStart > d->start)
        refreshStart = d->start;
      if (refreshStop < d->end)
        refreshStop = d->end;

      if (state == STATE_NONE)
      {
        SPISend(&lines[refreshStart], (refreshStop - refreshStart + 1)
                * sizeof(struct _linebuf));

        refreshStart = 0xff;
        refreshStop = 0;
      }
    }
    else if (ev == clear_event)
    {
      if (state == STATE_NONE)
      {
        SPISend(clear_cmd, 2);

        refreshStart = 0xff;
        refreshStop = 0;
      }
    }
  }

  PROCESS_END();
}

static void halLcdPixelInternal(int x,  int y, unsigned char style)
{
  if (style == STYLE_BLACK)
  {
    // if 0
    lines[y].pixels[x/8] &= ~(1 << (x & 0x07));
  }
  else if (style == STYLE_WHITE)
  {
    lines[y].pixels[x/8] |= 1 << (x & 0x07);
  }
  else if (style == STYLE_XOR)
  {
    lines[y].pixels[x/8] ^= 1 << (x & 0x07);
  }
}

void halLcdPixel(int x,  int y, unsigned char style)
{
  halLcdPixelInternal(x, y, style);
  halLcdRefresh(y, y);
}

void halLcdActive()
{
  P8OUT |= BIT2; // set 1 to active display
}

// write a string to display, truncated after 12 characters
// input: text		0-terminated string
//        line		vertical position of text, 0-95
//        options	can be combined using OR
//					DISP_INVERT	inverted text
//					DISP_WIDE double-width text (except for SPACE)
//					DISP_HIGH double-height text
void halLcdPrintXY(char text[], int col, int line, unsigned char options)
{
  // c = char
  // b = bitmap
  // i = text index
  // j = line buffer index
  // k = char line
  unsigned char c, b, i, j, k;
  unsigned char *LineBuff;

  // rendering happens line-by-line because this display can only be written by line
  k = 0;
  while(k < 8 && line < LCD_COL)						// loop for 8 character lines while within display
  {
    i = 0;
    j = col/8;
    while(j < (LCD_ROW/8) && (c = text[i]) != 0)	// while we did not reach end of line or string
    {
      LineBuff = lines[line + k].pixels;

      if(c < ' ')						// invalid characters are replace with SPACE
      {
        c = ' ';
      }

      if (c > 0x7f)
      {
        c = '?';
      }

      c = c - 32;									// convert character to index in font table
      b = font8x8[(c*8)+k];						// retrieve byte defining one line of character

      if(!(options & INVERT_TEXT))				// invert bits if DISP_INVERT is _NOT_ selected
      {											// pixels are LOW active
        b = ~b;
      }

      if((options & WIDE_TEXT) && (c != 0) && (j + 1 < LCD_ROW/8))		// double width rendering if DISP_WIDE and character is not SPACE
      {
        doubleWideAsm(b, &LineBuff[j]);			// implemented in assembly for efficiency/space reasons
        j += 2;									// we've written two bytes to buffer
      }
      else										// else regular rendering
      {
        LineBuff[j] = b;						// store pixels in line buffer
        j++;									// we've written one byte to buffer
      }

      i++;										// next character
    }

    if(options & HIGH_TEXT && (line + k + 1) < LCD_COL)	// repeat line if DISP_HIGH is selected
    {
      unsigned char *LineBuff1 = lines[line+k+1].pixels;
      unsigned char f;
      for(f = col/8; f < j; f++)
      {
        LineBuff1[f] = LineBuff[f];
      }
    }

    k++;											// next pixel line
  }

  halLcdRefresh(line, line + k - 1);
}

/**********************************************************************//**
 * @brief  Draws a horizontral line from (x1,y) to (x2,y) of style
 *
 * @param  x1        x-coordinate of the first point
 *
 * @param  x2        x-coordinate of the second point
 *
 * @param  y         y-coordinate of both points
 *
 * @param  style    Style of the horizontal line
 *
 * @return none
 *************************************************************************/
void halLcdHLine(int x1, int x2, int y, int width, unsigned char style)
{
    int x;

    if (x1 > x2)
    {
      int tmp = x2;
      x2 = x1;
      x1 = tmp;
    }

    for(int i = 0; i < width; i++)
    {
      // draw the first non-8-align pixels
      for(x = 0; x <= (x1 & 0x07); x++)
      {
        halLcdPixelInternal(x + x1, y + i, style);
      }

      for (x = (x1 + 7) & 0xF8; x+8 < x2; x+=8)
      {
        switch(style)
        {
        case 0:
          lines[y+i].pixels[x>>3] = 0;
          break;
        case 1:
          lines[y+i].pixels[x>>3] = 0xff;
          break;
        case 2:
          lines[y+i].pixels[x>>3] ^= 0xff;
          break;
        }
      }

      for(;x < x2; x++)
      {
        halLcdPixelInternal(x, y + i, style);
      }
    }
    halLcdRefresh(y, y + width - 1);
}

/**********************************************************************//**
 * @brief  Draws a vertical line from (x,y1) to (x,y2) of Style
 *
 * @param  x         x-coordinate of both points
 *
 * @param  y1        y-coordinate of the first point
 *
 * @param  y2        y-coordinate of the second point
 *
 * @param  style    Style of the vertical line
 *
 * @return none
 *************************************************************************/
void halLcdVLine(int x, int y1, int y2, int width, unsigned char style)
{
  int y;

  if (y1 > y2)
  {
    int tmp = y2;
    y2 = y1;
    y1 = tmp;
  }
  y = y1;
  for(int i = 0; i < width; i++)
  {
    while (y != y2)
    {
      halLcdPixelInternal(x + i, y, style);
      y ++;
    }
  }
  if (y2 > y1)
    halLcdRefresh(y1, y2);
  else
    halLcdRefresh(y2, y1);
}
/**********************************************************************//**
 * @brief  Draws a line from (x1,y1) to (x2,y2) of style.
 *
 * Uses Bresenham's line algorithm.
 *
 * @param  x1         x-coordinate of the first point
 *
 * @param  y1         y-coordinate of the first point
 *
 * @param  x2         x-coordinate of the second point
 *
 * @param  y2         y-coordinate of the second point
 *
 * @param  style 	  style of the line
 *
 * @return none
 *************************************************************************/
void halLcdLine(int x1, int y1, int x2, int y2, int width, unsigned char style)
{
    int x, y, deltay, deltax, d;
    int x_dir, y_dir;

    if (x1 == x2)
    {
        halLcdVLine(x1, y1, y2, width, style);
    }
    else
    {
        if (y1 == y2)
        {
          halLcdHLine(x1, x2, y1, width, style);
        }
        else                                // a diagonal line
        {
            if (x1 > x2)
                x_dir = -1;
            else x_dir = 1;
            if (y1 > y2)
                y_dir = -1;
            else y_dir = 1;

            x = x1;
            y = y1;
            deltay = ABS(y2 - y1);
            deltax = ABS(x2 - x1);

            if (deltax >= deltay)
            {
                d = (deltay << 1) - deltax;
                while (x != x2)
                {
                  for (int i = 0; i < width; i++)
                  {
                      halLcdPixelInternal(x, y + i, style);
                  }
                    if (d < 0)
                        d += (deltay << 1);
                    else
                    {
                        d += ((deltay - deltax) << 1);
                        y += y_dir;
                    }
                    x += x_dir;
                }
            }
            else
            {
                d = (deltax << 1) - deltay;
                while (y != y2)
                {
                  for (int i = 0; i < width; i++)
                  {
                    halLcdPixelInternal(x + i, y, style);
                  }
                    if (d < 0)
                        d += (deltax << 1);
                    else
                    {
                        d += ((deltax - deltay) << 1);
                        x += x_dir;
                    }
                    y += y_dir;
                }
            }
            if (y2 > y1)
              halLcdRefresh(y1, y2 + width);
            else
              halLcdRefresh(y2, y1 + width);
        }
    }
}

static inline void _draw_circle_8(int xc, int yc, int x, int y, unsigned char c) {
    // 参数 c 为颜色值
    halLcdPixelInternal(xc + x, yc + y, c);
    halLcdPixelInternal(xc - x, yc + y, c);
    halLcdPixelInternal(xc + x, yc - y, c);
    halLcdPixelInternal(xc - x, yc - y, c);
    halLcdPixelInternal(xc + y, yc + x, c);
    halLcdPixelInternal(xc - y, yc + x, c);
    halLcdPixelInternal(xc + y, yc - x, c);
    halLcdPixelInternal(xc - y, yc - x, c);
}


/**********************************************************************//**
 * @brief  Draw a circle of Radius with center at (x,y) of GrayScale level.
 *
 * Uses Bresenham's circle algorithm
 *
 * @param  x         x-coordinate of the circle's center point
 *
 * @param  y         y-coordinate of the circle's center point
 *
 * @param  Radius    Radius of the circle
 *
 * @param  style  style of the circle
 *************************************************************************/
//Bresenham's circle algorithm
void halLcdCircle(int xc, int yc, int r, int fill, unsigned char style) {
    // (xc, yc) 为圆心，r 为半径
    // fill 为是否填充
    // c 为颜色值

    int x = 0, y = r, yi, d;
    d = 3 - 2 * r;

    if (fill) {
        // 如果填充（画实心圆）
        while (x <= y) {
            for (yi = x; yi <= y; yi ++)
                _draw_circle_8(xc, yc, x, yi, style);

            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y --;
            }
            x++;
        }
    } else {
        // 如果不填充（画空心圆）
        while (x <= y) {
            _draw_circle_8(xc, yc, x, y, style);

            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y --;
            }
            x ++;
        }
    }

    halLcdRefresh(yc - r, yc + r);
}