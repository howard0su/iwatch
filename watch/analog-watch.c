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
#include "hal_lcd.h"
#include "window.h"
#include "rtc.h"
#include "math.h"

/*
 * This implement the digit watch
 * Wake up every 1 second and update the watch
 * If in 10 minutes, no key or other things
 * if get system key in non-suspend state, post event to system.
 */
#define PI 3.1415927
#define CENTER_X 72
#define CENTER_Y 84

#define SEC_HAND_LEN 70
#define MIN_HAND_LEN 60
#define HOUR_HAND_LEN 45

static uint16_t lastSecX, lastSecY;
static uint16_t lastMinX, lastMinY;
static uint16_t lastHourX, lastHourY;

static void drawBackground()
{
  halLcdBeginUpdate();

  halLcdClearScreen();

  halLcdVLine(72, 0, 9, 2, 0);
  halLcdVLine(72, 158, 167, 2, 0);
  halLcdHLine(0, 9, 84, 2, 0);
  halLcdHLine(134, 143, 84, 2, 0);

  halLcdEndUpdate();
  lastSecX = CENTER_X;
  lastSecY = CENTER_Y;
  lastMinX = CENTER_X;
  lastMinY = CENTER_Y;
  lastHourX = CENTER_X;
  lastHourY = CENTER_Y;
}

static void drawClock(int day, int h, int m, int s)
{
  float fangle;
  int angle;
  uint16_t x, y;
  char buf[] = "00";
  halLcdBeginUpdate();

  if (s > 0)
  {
    // sec hand: length = 75
    angle = s * 6;
    fangle = (360 - angle) * PI/180;
    halLcdLine(CENTER_X, CENTER_Y, lastSecX , lastSecY, 1, 1);
    lastSecX = CENTER_X + SEC_HAND_LEN * sin(fangle);
    lastSecY = CENTER_Y + SEC_HAND_LEN * cos(fangle);
    halLcdLine(CENTER_X, CENTER_Y, lastSecX, lastSecY, 1, 0);
  }

  // minute hand = length = 70
  angle = m*6+s/10;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + MIN_HAND_LEN * sin(fangle);
  if (x != lastMinX)
  {
    y = CENTER_Y + MIN_HAND_LEN * cos(fangle);
    halLcdLine(CENTER_X, CENTER_Y,  lastMinX, lastMinY, 3, 1);
    halLcdLine(CENTER_X, CENTER_Y,  x, y, 3, 0);
    lastMinX = x;
    lastMinY = y;
  }
  else
  {
    halLcdLine(CENTER_X, CENTER_Y,  lastMinX, lastMinY, 3, 0);
  }

  // hour hand 45
  angle = h*30 + m/2;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + HOUR_HAND_LEN * sin(fangle);
  if (x != lastHourX)
  {
    y = CENTER_Y + HOUR_HAND_LEN * cos(fangle);
    halLcdLine(CENTER_X, CENTER_Y, lastHourX, lastHourY, 5, 1);
    halLcdLine(CENTER_X, CENTER_Y, x, y, 5, 0);
    lastHourX = x;
    lastHourY = y;
  }
  else
  {
    halLcdLine(CENTER_X, CENTER_Y,  lastHourX, lastHourY, 5, 0);
  }

  buf[0] += day >> 4;
  buf[1] += day & 0x0f;
  halLcdHLine(93, 118, 72, 18, 0);
  halLcdPrintXY(buf, 98, 77, INVERT_TEXT);

  halLcdCircle(CENTER_X, CENTER_Y, 5, 1, 0);

  halLcdEndUpdate();
}

PROCESS(analogclock_process, "Analog Clock Window");

PROCESS_THREAD(analogclock_process, ev, data)
{
  PROCESS_BEGIN();

  drawBackground();
  rtc_enablechange(SECOND_CHANGE);

  while(1)
  {
    if (ev == EVENT_TIME_CHANGED)
    {
      struct datetime* dt = (struct datetime*)data;
      drawClock(dt->day, dt->hour, dt->minute, dt->second);
    }
    else
      window_defproc(ev, data);

    PROCESS_WAIT_EVENT();
  }
  PROCESS_END();
}