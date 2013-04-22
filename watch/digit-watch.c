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

static void drawClock(struct datetime *dt)
{
  char buf[2] = "00";

  GrContextForegroundSet(&context, COLOR_WHITE);
  GrContextFontSet(&context, &g_sFontNova30b);

  buf[0] = '0' + dt->hour / 10;
  buf[1] = '0' + dt->hour % 10;

  GrStringDraw(&context, buf, 2, 68, 77, 0);

  buf[0] = '0' + dt->minute / 10;
  buf[1] = '0' + dt->minute % 10;

  GrStringDraw(&context, buf, 2, 98, 77, 0);

  GrFlush(&context);
}

uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  static int firsttime;

  if (ev == EVENT_WINDOW_CREATED)
  {
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

    drawClock(dt);
  }
  else
  {
    return 0;
  }

  return 1;
}