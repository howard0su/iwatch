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

extern void mpu6050_init();
extern void ant_init();
extern void bluetooth_init();
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

{
  PROCESS_BEGIN();

  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);

  GrContextForegroundSet(&context, COLOR_BLACK);
  GrContextBackgroundSet(&context, COLOR_WHITE);
  tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
  GrRectFill(&context, &rect);
  GrContextFontSet(&context, &g_sFontCm44i);
  GrContextForegroundSet(&context, COLOR_WHITE);
  GrStringDraw(&context, "iWatch", -1, 10, 58, 0);
  GrFlush(&context);
  window_open(&menu_process, NULL);

  // give time to starts
  button_init();
  rtc_init();
  I2C_Init();

  //ant_init();
  bluetooth_init();
  mpu6050_init();

  while(1)
  {

  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
