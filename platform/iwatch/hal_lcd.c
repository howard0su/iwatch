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

static process_event_t refresh_event;

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
  struct RefreshData data;
  
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
  
  PRINTF("Update from Line %d to Line %d\n", line, line + k - 1);
  
  data.start = line;
  data.end = line + k - 1;
  process_post_synch(&lcd_process, refresh_event, &data);
}

static void SPIInit()
{
  UCB0CTL1 = UCSWRST;
  
  UCB0CTL0 = UCMODE0 + UCMST + UCSYNC + UCCKPH; // master, 3-pin SPI mode, LSB
  UCB0CTL1 |= UCSSEL__SMCLK; // SMCLK for now
  UCB0BR0 = 16; // 16MHZ / 16 = 1Mhz
  UCB0BR1 = 0;
  
  //Configure ports.
  SPIDIR |= _SCLK | _SDATA | _SCS;
  SPISEL |= _SCLK | _SDATA;
  SPIOUT &= ~(_SCLK | _SDATA | _SCS);
  
  UCB0CTL1 &= ~UCSWRST;
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
  
  // wait spi stable
  __delay_cycles(100);
    
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
  struct RefreshData data;
  
  for(i = 0; i < LCD_COL; i++)
  {
    memset(lines[i].pixels, 0, LCD_ROW/8);
  }
  data.start = 0;
  data.end = LCD_COL - 1;
  process_post_synch(&lcd_process, refresh_event, &data);
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

PROCESS_THREAD(lcd_process, ev, data)
{
  static unsigned char refreshStart = 0xff, refreshStop = 0;
  
  PROCESS_BEGIN();
  
  refresh_event = process_alloc_event();
  
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
  }
  
  PROCESS_END();
}
void halLcdActive()
{
  P8OUT |= BIT2; // set 1 to active display
}