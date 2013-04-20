/****************************************************************
*  Description: Implementation for System process
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
#include "string.h"
#include "lib/print-stats.h"
#include "sys/clock.h"
#include "lib/sensors.h"
#include "button.h"
#include "rtc.h"
#include "window.h"

#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "btstack/bluetooth.h"
#include "backlight.h"

extern void mpu6050_init();
extern void ant_init();
extern void button_init();
extern void I2C_Init();

#include <stdlib.h>
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/

extern tContext context;

/*
* This process is the startup process.
* It first shows the logo
* Like the whole dialog intrufstture.
*/
uint8_t watch_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    {
      backlight_on(255);
      GrContextForegroundSet(&context, COLOR_BLACK);
      tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
      GrRectFill(&context, &rect);
      GrContextFontSet(&context, &g_sFontNova30b);
      GrContextForegroundSet(&context, COLOR_WHITE);
      GrStringDraw(&context, "iWatch", -1, 10, 58, 0);
      GrFlush(&context);

      if (bluetooth_paired())
      {
        bluetooth_init();
      }

      window_timer(CLOCK_SECOND);
      break;
    }
  case PROCESS_EVENT_TIMER:
    {
      backlight_on(0);
      window_open(&menu_process, NULL);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    {
      break;
    }
  default:
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
