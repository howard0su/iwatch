/****************************************************************
*  Description: Implementation for Analog watch Window
*    History:
*      Jun Su          2013/1/2        Created
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
#include "window.h"
#include "rtc.h"
#include "math.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "memory.h"

#include <stdio.h> // for sprintf
#include <string.h>

#define _hour0 d.digit.hour0
#define _minute d.digit.minute
static uint8_t _selection;

typedef void (*draw_function)(tContext *pContext);

static void drawClock0(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ampm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit44);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE/2, 50, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, 100, 65, 0);

  GrContextFontSet(pContext, &g_sFontNova13);
  sprintf(buf, "%s %d, %d", month_name[month - 1], day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE/2, 135, 0);
}

static void drawClock1(tContext *pContext)
{
  uint8_t ampm = 0;
  uint8_t hour = _hour0;
  char buf[20];

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit56);

  sprintf(buf, "%02d", hour);
  GrStringDrawCentered(pContext, buf, 2, LCD_X_SIZE/ 2, 45, 0);

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit52b);
  sprintf(buf, "%02d", _minute);
  GrStringDrawCentered(pContext, buf, 2, LCD_X_SIZE / 2, 90, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDrawCentered(pContext, buf, 2, LCD_X_SIZE / 2, 130, 0);
}

static void drawClock2(tContext *pContext)
{
  uint8_t ampm = 0;
  uint8_t hour = _hour0;
  char buf[20];
  const char* buffer;

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova38);
  buffer = toEnglish(hour, buf);
  GrStringDraw(pContext, buffer, -1, 10, 50, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  buffer = toEnglish(_minute, buf);
  GrStringDraw(pContext, buffer, -1, 10, 85, 0);

  GrContextFontSet(pContext, &g_sFontNova16);
  if (ampm)
  {
    GrStringDraw(pContext, "In the PM", -1, 10, 100, 0);
  }
  else
  {
    GrStringDraw(pContext, "In the AM", -1, 10, 100, 0);
  }
}


static void drawClock3(tContext *pContext)
{
  uint8_t ampm = 0;
  uint8_t hour = _hour0;
  char buf[20];
  const char* buffer;

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova38);

  buffer = toEnglish(hour, buf);
  tRectangle rect = {8, 28, LCD_X_SIZE-8, 65};
  GrRectFillRound(pContext, &rect, 8);
  GrContextForegroundSet(pContext, ClrBlack);
  GrStringDrawCentered(pContext, buffer, -1, LCD_X_SIZE/ 2, 45, 0);
  GrContextForegroundSet(pContext, ClrWhite);
  
  GrContextFontSet(pContext, &g_sFontNova16b);
  buffer = toEnglish(_minute, buf);
  GrStringDrawCentered(pContext, buffer, -1, LCD_X_SIZE / 2, 80, 0);

  GrContextFontSet(pContext, &g_sFontNova16);
  if (ampm)
  {
    GrStringDrawCentered(pContext, "In the PM", -1, LCD_X_SIZE / 2, 120, 0);
  }
  else
  {
    GrStringDrawCentered(pContext, "In the AM", -1, LCD_X_SIZE / 2, 120, 0);
  }
}

static void drawClock8(tContext *pContext)
{
    GrContextForegroundSet(pContext, ClrWhite);
    GrRectFill(pContext, &client_clip);
    GrContextForegroundSet(pContext, ClrBlack);

    drawClock0(pContext);
}

static void drawClock4(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit56);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 65, 0);

  GrContextFontSet(pContext, &g_sFontNova16);
  sprintf(buf, "%s %d, %d", month_name[month - 1], day, year);
  tRectangle rect = {8, 100, LCD_X_SIZE-8, 120};
  GrRectFillRound(pContext, &rect, 6);
  GrContextForegroundSet(pContext, ClrBlack);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 108, 0);
}

static void drawClock5(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ampm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit44b);

  sprintf(buf, "%02d:%02d", hour, _minute);  
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 65, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, 105, 80, 0);

  GrContextFontSet(pContext, &g_sFontNova16);
  sprintf(buf, "%02d-%02d-%02d", month, day, year - 2000);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 135, 0);
}

static void drawClock6(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  tRectangle rect = {30, 20, LCD_X_SIZE - 30, LCD_Y_SIZE - 40};
  GrRectFillRound(pContext, &rect, 8);
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit52b);
  sprintf(buf, "%02d", hour);
  GrStringDrawCentered(pContext, buf, 2, LCD_X_SIZE / 2, 50, 0);

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit56);
  sprintf(buf, "%02d", _minute);
  GrStringDrawCentered(pContext, buf, 2, LCD_X_SIZE / 2, 95, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  GrContextForegroundSet(pContext, ClrWhite);
  sprintf(buf, "%s %d, %d", month_name[month - 1], day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 135, 0);
}


static void drawClock7(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ampm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, (tFont*)&g_sFontExDigit44b);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 70, 0);

  GrContextFontSet(pContext, &g_sFontNova16b);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, 105, 85, 0);

  GrContextFontSet(pContext, &g_sFontNova13);
  sprintf(buf, "%s %d, %d", month_name[month - 1], day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 35, 0);
}

static const draw_function Clock_selections[] =
{
  drawClock0,
  drawClock1,
  drawClock2,
  drawClock3,
  drawClock4,
  drawClock5,
  drawClock6,
  drawClock7,
  drawClock8
};

uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&_hour0, &_minute, NULL);
    if (rparam == NULL)
      _selection = window_readconfig()->digit_clock;
    else
      _selection = (uint8_t)rparam - 1;
    rtc_enablechange(MINUTE_CHANGE);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    // clear the region
    tContext *pContext = (tContext*)rparam;
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &client_clip);
    GrContextForegroundSet(pContext, ClrWhite);

    Clock_selections[_selection](pContext);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    _hour0 = dt->hour;
    _minute = dt->minute;
    window_invalid(NULL);
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_DOWN)
    {
      _selection += 0x1;
      if (_selection > sizeof(Clock_selections)/sizeof(draw_function) - 1)
      {
        _selection = 0x00;
      }
      window_invalid(NULL);
    }
    else if (lparam == KEY_UP)
    {
      _selection -= 0x1;
      if (_selection == 0xff)
      {
        _selection = sizeof(Clock_selections)/sizeof(draw_function) - 1;
      }
      window_invalid(NULL);
    }
  }
  else if (ev == EVENT_WINDOW_CLOSING)
  {
    rtc_enablechange(0);

    window_readconfig()->default_clock = 1;
    if (_selection != window_readconfig()->digit_clock)
    {
      window_readconfig()->digit_clock = _selection;
      window_writeconfig();
    }
  }
  else
  {
    return 0;
  }

  return 1;
}