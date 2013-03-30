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
  tContext context;
  GrContextInit(&context, &g_memlcd_Driver);

  GrClearDisplay(&context);

  const static tRectangle rect1 = {72, 0, 74, 9};
  const static tRectangle rect2 = {72, 158, 74, 167};
  const static tRectangle rect3 = {0, 84, 9, 86};
  const static tRectangle rect4 = {134, 84, 143, 86};

  GrRectFill(&context, &rect1);
  GrRectFill(&context, &rect2);
  GrRectFill(&context, &rect3);
  GrRectFill(&context, &rect4);

  GrFlush(&context);
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
  tContext context;
  GrContextInit(&context, &g_memlcd_Driver);

  if (s > 0)
  {
    // sec hand: length = 75
    angle = s * 6;
    fangle = (360 - angle) * PI/180;
    GrContextForegroundSet(&context, 2); // xor
    GrLineDraw(&context, CENTER_X, CENTER_Y, lastSecX , lastSecY);
    lastSecX = CENTER_X + SEC_HAND_LEN * sin(fangle);
    lastSecY = CENTER_Y + SEC_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, 0);
    GrLineDraw(&context, CENTER_X, CENTER_Y, lastSecX , lastSecY);
  }

  // minute hand = length = 70
  angle = m*6+s/10;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + MIN_HAND_LEN * sin(fangle);
  if (x != lastMinX)
  {
    y = CENTER_Y + MIN_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, 2); // xor
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastMinX, lastMinY);
    GrContextForegroundSet(&context, 0);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  x, y);
    lastMinX = x;
    lastMinY = y;
  }
  else
  {
    GrContextForegroundSet(&context, 0);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastMinX, lastMinY);
  }

  // hour hand 45
  angle = h*30 + m/2;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + HOUR_HAND_LEN * sin(fangle);
  if (x != lastHourX)
  {
    y = CENTER_Y + HOUR_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, 2); // xor
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastHourX, lastHourY);
    GrContextForegroundSet(&context, 0);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  x, y);
    lastHourX = x;
    lastHourY = y;
  }
  else
  {
    GrContextForegroundSet(&context, 0);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastHourX, lastHourY);
  }

  GrCircleFill(&context, CENTER_X, CENTER_Y, 5);

  buf[0] += day >> 4;
  buf[1] += day & 0x0f;

  GrContextForegroundSet(&context, 1);
  GrStringDraw(&context, buf, 2, 98, 77, 1);

  GrFlush(&context);
}

PROCESS(analogclock_process, "Analog Clock Window");

PROCESS_THREAD(analogclock_process, ev, data)
{
  PROCESS_BEGIN();

  rtc_enablechange(SECOND_CHANGE);
  PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_TIME_CHANGED);
  {
    struct datetime* dt = (struct datetime*)data;
    drawBackground();
    drawClock(dt->day, dt->hour, dt->minute, -1);
  }
  rtc_enablechange(MINUTE_CHANGE);

  while(1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == EVENT_TIME_CHANGED)
    {
      struct datetime* dt = (struct datetime*)data;
      drawClock(dt->day, dt->hour, dt->minute, -1);
    }
    else
    {
      window_defproc(ev, data);
    }
  }
  PROCESS_END();
}