/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 * Author  : Adam Dunkels
 */

#include "lcd.h"
#include "hal_lcd.h"

#define WITH_LCD 1

#define X_CHAR_SIZE 8
#define Y_CHAR_SIZE 9

#define X_MAX (LCD_MAX_X/X_CHAR_SIZE)
#define Y_MAX (LCD_MAX_Y/Y_CHAR_SIZE)

static unsigned char ypos;

static char line[X_MAX + 1];
static unsigned char xpos;

#define SCROLL_AREA LCD_MAX_SCROLL_AREA
/*---------------------------------------------------------------------------*/
void
lcd_init(void)
{
  if(WITH_LCD) {
    halLcdInit();
    halLcdActive();
    halLcdBackLightInit();
    halLcdSetBackLight(8);
    lcd_clear();
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_clear(void)
{
  if(WITH_LCD) {
    halLcdClearScreen();
    xpos = ypos = 0;
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_set_pixel(int x, int y, int intensity)
{
  if(WITH_LCD) {
    halLcdPixel(x, y, intensity);
  }
}
/*---------------------------------------------------------------------------*/
static void
inc_y(void)
{
  if(WITH_LCD) {
    ypos++;
    xpos = 0;
    line[xpos] = 0;
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_write_char(char c)
{
  if(WITH_LCD) {
    if(c == '\n') {
      halLcdPrintXY(line, 0, ypos * Y_CHAR_SIZE, 0);
      inc_y();
    } else {
      line[xpos] = c;
      line[xpos + 1] = 0;

      if(ypos == Y_MAX) {
	lcd_clear();
        xpos = 0;
	ypos = 0;
        line[xpos] = 0;
      }

      if(xpos == X_MAX) {
        halLcdPrintXY(line, 0, ypos * Y_CHAR_SIZE, 0);
	inc_y();
      } else {
	xpos++;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_draw_line(int x0, int y0, int x1, int y1)
{
  if(WITH_LCD) {
    halLcdLine(x0, y0, x1, y1, 3);
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_draw_vertical_line(int pixels)
{
  if(WITH_LCD) {
    if(pixels > SCROLL_AREA) {
      pixels = SCROLL_AREA;
    }
    lcd_draw_line(LCD_MAX_X, LCD_MAX_Y - pixels, LCD_MAX_X, LCD_MAX_Y);
  }
}
/*---------------------------------------------------------------------------*/
void
lcd_scroll_x(void)
{
  if(WITH_LCD) {
    halLcdHScroll(LCD_MAX_Y - SCROLL_AREA, LCD_MAX_Y);
    halLcdLine(LCD_MAX_X, LCD_MAX_Y - SCROLL_AREA, LCD_MAX_X, LCD_MAX_Y, 0);
  }
}
/*---------------------------------------------------------------------------*/
