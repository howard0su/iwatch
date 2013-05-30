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

#include <stdio.h> // for sprintf
#include <string.h>

static uint8_t hour, minute;
static uint8_t selection;

static const char *month_name[] =
{
  "January", "February", "March", "April", "May",
  "June", "July", "August", "September",
  "November", "December"
};

static const char* english_name[] =
{
  "ZERO", "ONE", "TWO", "THREE", "FOUR", "FIVE", "SIX", "SEVEN", "EIGHT",
  "NINE", "TEN", "ELEVEN", "TWELEVE", "THIRTEEN", "FOURTEEN",
  "FIFTEEN", "SIXTEEN", "SEVENTEEN", "EIGHTEEN", "NINETEEN"
};

static const char* english_name_prefix[] =
{
    "TWENTY", "THIRTY", "FOURTY", "FIVETY", "SIXTY",
    "SEVENTY", "EIGHTY", "NINTY"
};

// Only support number less than 100
static const char* toEnglish(uint8_t number, char* buffer)
{
  if (number < 20)
  {
    return english_name[number];
  }

  // larger than 20
  strcpy(buffer, english_name_prefix[(number / 10) - 2]);
  if (number % 10 != 0)
  {
    strcat(buffer, " ");
    strcat(buffer, english_name[number % 10]);
  }

  return buffer;
}

typedef void (*draw_function)(tContext *pContext);

static void drawClock0(tContext *pContext)
{
  uint16_t year;
  uint8_t month, day, width;
  uint8_t ampm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova38b);

  sprintf(buf, "%02d : %02d", hour, minute);
  width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, -1, (LCD_X_SIZE - width) / 2, 30, 0);

  GrContextFontSet(pContext, &g_sFontNova28b);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, 90, 60, 0);

  GrContextFontSet(pContext, &g_sFontNova12);
  sprintf(buf, "%s %d, %d", month_name[month], day, year);
  width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, -1, (LCD_X_SIZE - width) / 2, 135, 0);
}

static void drawClock1(tContext *pContext)
{
  uint8_t ampm = 0;
  uint8_t width;
  char buf[20];

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova38b);

  sprintf(buf, "%02d", hour);
  width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, 2, (LCD_X_SIZE - width) / 2, 30, 0);

  sprintf(buf, "%02d", minute);
  width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, 2, (LCD_X_SIZE - width) / 2, 70, 0);

  GrContextFontSet(pContext, &g_sFontNova28);
  if (ampm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, 2, (LCD_X_SIZE - width) / 2, 110, 0);
}

static void drawClock2(tContext *pContext)
{
  uint8_t ampm = 0;
  char buf[20];
  const char* buffer;

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova28);

  buffer = toEnglish(hour, buf);
  GrStringDraw(pContext, buffer, -1, 10, 30, 0);

  GrContextFontSet(pContext, &g_sFontNova12b);
  buffer = toEnglish(minute, buf);
  GrStringDraw(pContext, buffer, -1, 10, 70, 0);

  GrContextFontSet(pContext, &g_sFontNova12);
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
  uint8_t width;
  char buf[20];
  const char* buffer;

  // draw time
  if (hour > 12)
  {
    ampm = 1; // pm
    hour -= 12;
  }

  GrContextFontSet(pContext, &g_sFontNova28);

  buffer = toEnglish(hour, buf);
  width = GrStringWidthGet(pContext, buffer, -1);
  GrStringDraw(pContext, buffer, -1, (LCD_X_SIZE - width) / 2, 30, 0);

  GrContextFontSet(pContext, &g_sFontNova12b);
  buffer = toEnglish(minute, buf);
  width = GrStringWidthGet(pContext, buffer, -1);
  GrStringDraw(pContext, buffer, -1, (LCD_X_SIZE - width) / 2, 70, 0);

  GrContextFontSet(pContext, &g_sFontNova12);
  if (!ampm)
  {
    width = GrStringWidthGet(pContext, "In the PM", -1);
    GrStringDraw(pContext, "In the PM", -1, (LCD_X_SIZE - width) / 2, 120, 0);
  }
  else
  {
    width = GrStringWidthGet(pContext, "In the AM", -1);
    GrStringDraw(pContext, "In the AM", -1, (LCD_X_SIZE - width) / 2, 120, 0);
  }
}

draw_function ClockSelections[] =
{
  drawClock0,
  drawClock1,
  drawClock2,
  drawClock3
};

uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&hour, &minute, NULL);
    if (rparam == NULL)
      selection = window_readconfig()->digit_clock;
    else
      selection = (uint8_t)rparam - 1;
    rtc_enablechange(MINUTE_CHANGE);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    // clear the region
    tContext *pContext = (tContext*)rparam;
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &client_clip);
    GrContextForegroundSet(pContext, ClrWhite);

    ClockSelections[selection](pContext);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    hour = dt->hour;
    minute = dt->minute;
    window_invalid(NULL);
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_ENTER)
    {
      selection += 0x1;
      if (selection > 0x03)
      {
        selection = 0x00;
      }
      window_invalid(NULL);
    }
  }
  else if (ev == EVENT_WINDOW_CLOSING)
  {
    rtc_enablechange(0);

    if (selection != window_readconfig()->digit_clock)
    {
      ui_config config;
      memcpy(&config, window_readconfig(), sizeof(config));
      config.digit_clock = selection;
      window_writeconfig(&config);
    }
  }
  else
  {
    return 0;
  }

  return 1;
}