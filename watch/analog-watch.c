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
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "cordic.h"
#define SUPPORT_SECOND

/*
 * This implement the digit watch
 * Wake up every 1 second and update the watch
 * If in 10 minutes, no key or other things
 * if get system key in non-suspend state, post event to system.
 */
#define CENTER_X 72
#define CENTER_Y 84

#define SEC_HAND_LEN 70
#define MIN_HAND_LEN 60
#define HOUR_HAND_LEN 45

static uint8_t hour, minute, sec;

static void drawFace(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  const static tRectangle rect1 = {72, 16, 74, 25};
  const static tRectangle rect2 = {72, 158, 74, 167};
  const static tRectangle rect3 = {0, 84, 9, 86};
  const static tRectangle rect4 = {134, 84, 143, 86};

  GrRectFill(pContext, &rect1);
  GrRectFill(pContext, &rect2);
  GrRectFill(pContext, &rect3);
  GrRectFill(pContext, &rect4);
}

static void drawHands(tContext *pContext, int h, int m, int s)
{
  int cos_val, sin_val;
  int angle;
  uint16_t x, y;

#ifdef SUPPORT_SECOND
  if (s > 0)
  {
    // sec hand: length = 75
    angle = s * 6;
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + ((SEC_HAND_LEN * (sin_val >> 8)) >> 7);
    y = CENTER_Y - ((SEC_HAND_LEN * (cos_val >> 8)) >> 7);
    GrContextForegroundSet(pContext, ClrWhite);
    GrLineDraw(pContext, CENTER_X, CENTER_Y, x, y);
  }
#endif

  // minute hand = length = 70
  angle = m*6+s/10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);

    // hour hand 45
  angle = h*30 + m/2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 5);
}

uint8_t analogclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&hour, &minute, &sec);
    rtc_enablechange(SECOND_CHANGE);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    drawFace((tContext*)rparam);
    drawHands((tContext*)rparam, hour, minute, sec);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    hour = dt->hour;
    if (hour > 12)
      hour -= 12;
    minute = dt->minute;
    sec = dt->second;
    window_invalid(NULL);
  }
  else
  {
    return 0;
  }

  return 1;
}