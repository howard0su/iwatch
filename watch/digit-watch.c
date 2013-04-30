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

static uint8_t hour, minute, second;

static void drawClock(tContext *pContext)
{
  GrContextFontSet(pContext, &g_sFontNova28b);

  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  uint8_t times[3] = {hour, minute, second};
  window_drawtime(pContext, 45, times, 0);
}

uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&hour, &minute, &second);
    rtc_enablechange(SECOND_CHANGE);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    drawClock((tContext*)rparam);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    hour = dt->hour;
    minute = dt->minute;
    second = dt->second;
    tRectangle rect = {0, 63, LCD_Y_SIZE, 94};
    window_invalid(&rect);
  }
  else
  {
    return 0;
  }

  return 1;
}