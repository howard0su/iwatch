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

extern tContext context;
extern tRectangle client_clip;

static void drawBackground()
{
  GrContextForegroundSet(&context, COLOR_WHITE);

  const static tRectangle rect1 = {72, 16, 74, 25};
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
  char buf[2];

  if (s > 0)
  {
    // sec hand: length = 75
    angle = s * 6;
    fangle = (360 - angle) * PI/180;
    GrContextForegroundSet(&context, COLOR_BLACK);
    GrLineDraw(&context, CENTER_X, CENTER_Y, lastSecX , lastSecY);
    lastSecX = CENTER_X + SEC_HAND_LEN * sin(fangle);
    lastSecY = CENTER_Y + SEC_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrLineDraw(&context, CENTER_X, CENTER_Y, lastSecX , lastSecY);
  }

  // minute hand = length = 70
  angle = m*6+s/10;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + MIN_HAND_LEN * sin(fangle);
  if (x != lastMinX)
  {
    y = CENTER_Y + MIN_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, COLOR_BLACK); // xor
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastMinX, lastMinY);
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  x, y);
    lastMinX = x;
    lastMinY = y;
  }
  else
  {
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastMinX, lastMinY);
  }

  // hour hand 45
  angle = h*30 + m/2;
  fangle = (360 - angle) * PI/180;
  x = CENTER_X + HOUR_HAND_LEN * sin(fangle);
  if (x != lastHourX)
  {
    y = CENTER_Y + HOUR_HAND_LEN * cos(fangle);
    GrContextForegroundSet(&context, COLOR_BLACK); // xor
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastHourX, lastHourY);
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  x, y);
    lastHourX = x;
    lastHourY = y;
  }
  else
  {
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrLineDraw(&context, CENTER_X, CENTER_Y,  lastHourX, lastHourY);
  }

  GrCircleFill(&context, CENTER_X, CENTER_Y, 5);

  buf[0] = '0' + day >> 4;
  buf[1] = '0' + (day & 0x0f);

  GrContextForegroundSet(&context, COLOR_BLACK);
  GrContextBackgroundSet(&context, COLOR_WHITE);
  GrStringDraw(&context, buf, 2, 98, 77, 1);

  GrFlush(&context);
}

uint8_t analogclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  static int firsttime;

  if (ev == EVENT_WINDOW_CREATED)
  {
    GrContextForegroundSet(&context, COLOR_BLACK);
    GrRectFill(&context, &client_clip);

    rtc_enablechange(SECOND_CHANGE);
    firsttime = 1;
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    if (firsttime)
    {
      firsttime = 0;
      rtc_enablechange(MINUTE_CHANGE);
    }

    drawBackground();
    drawClock(dt->day, dt->hour, dt->minute, -1);
  }
  else
  {
    return 0;
  }

  return 1;
}