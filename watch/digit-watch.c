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

static uint8_t hour, minute;

static void drawClock(tContext *pContext)
{
  char buf[2] = "00";

  GrContextForegroundSet(pContext, COLOR_WHITE);
  GrContextFontSet(pContext, &g_sFontNova30b);

  buf[0] = '0' + hour / 10;
  buf[1] = '0' + hour % 10;

  GrStringDraw(pContext, buf, 2, 68, 77, 0);

  buf[0] = '0' + minute / 10;
  buf[1] = '0' + minute % 10;

  GrStringDraw(pContext, buf, 2, 98, 77, 0);
}

uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  static int firsttime;

  if (ev == EVENT_WINDOW_CREATED)
  {
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

    window_invalid(NULL);
  }
  else
  {
    return 0;
  }

  return 1;
}