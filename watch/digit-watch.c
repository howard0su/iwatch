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

extern tContext context;

static void drawClock(struct datetime *dt)
{
  char buf[2] = "00";

  GrContextForegroundSet(&context, COLOR_WHITE);
  GrContextFontSet(&context, &g_sFontCm32);

  buf[0] = '0' + dt->hour / 10;
  buf[1] = '0' + dt->hour % 10;

  GrStringDraw(&context, buf, 2, 68, 77, 0);

  buf[0] = '0' + dt->minute / 10;
  buf[1] = '0' + dt->minute % 10;

  GrStringDraw(&context, buf, 2, 98, 77, 0);

  GrFlush(&context);
}

PROCESS(digitclock_process, "Digit Clock Window");

PROCESS_THREAD(digitclock_process, ev, data)
{
  PROCESS_BEGIN();

  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == EVENT_WINDOW_CREATED)
    {
      rtc_enablechange(SECOND_CHANGE);
      PROCESS_WAIT_EVENT_UNTIL(ev == EVENT_TIME_CHANGED);
      {
        struct datetime* dt = (struct datetime*)data;
        tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
        GrRectFill(&context, &rect);

        drawClock(dt);
      }
      rtc_enablechange(MINUTE_CHANGE);
    }
    else if (ev == EVENT_TIME_CHANGED)
    {
      struct datetime* dt = (struct datetime*)data;
      drawClock(dt);
    }
    else
    {
      window_defproc(ev, data);
    }
  }
  PROCESS_END();
}