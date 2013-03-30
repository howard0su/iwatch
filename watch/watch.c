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
PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);
/*---------------------------------------------------------------------------*/

extern tContext context;

/*
* This process is the startup process.
* It first shows the logo
* Like the whole dialog intrufstture.
*/
PROCESS_THREAD(system_process, ev, data)
{
  PROCESS_BEGIN();
//  ui_process = &analogclock_process;
//  ui_process = &control_process;
  ui_process = &menu_process;
//  ui_process = PROCESS_CURRENT();
  button_init();
  rtc_init();

  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  GrContextFontSet(&context, &g_sFontCm12);
  GrClearDisplay(&context);

  // give time to starts
  I2C_Init();

  ant_init();
  bluetooth_init();
  mpu6050_init();

  window_open(ui_process, NULL);
#if 0
  while(1)
  {
    PROCESS_WAIT_EVENT();
    switch(ev)
    {
    case EVENT_BT_STATUS:
      {
        bt_status = (uint8_t)data;
        if (bt_status & BIT0)
        {
          GrStringDraw(&context, "BT OK", -1, 10, 2, 0);
          GrFlush(&context);
          printf("=============================BT OK=============================\n");
        }
        break;
      }
    case EVENT_ANT_STATUS:
      {
        bt_status = (uint8_t)data;
        if (bt_status & BIT0)
        {
          GrStringDraw(&context, "ANT OK", -1, 10, 18, 0);
          GrFlush(&context);
          printf("=============================ANT OK============================\n");
        }
        break;
      }
    case EVENT_MPU_STATUS:
      {
        bt_status = (uint8_t)data;
        if (bt_status & BIT0)
        {
          GrStringDraw(&context, "MPU OK", -1, 10, 26, 0);
          GrFlush(&context);
          printf("=============================MPU OK============================\n");
        }
        break;
      }
    case EVENT_CODEC_STATUS:
      {
        bt_status = (uint8_t)data;
        if (bt_status & BIT0)
        {
          GrStringDraw(&context, "CODEC OK", -1, 10, 42, 0);
          GrFlush(&context);
          printf("=============================CODEC OK==========================\n");
        }
        break;
      }
    case EVENT_KEY_PRESSED:
      {
        GrStringDraw(&context, "KEY OK", -1, 10, 58, 0);
        GrFlush(&context);

        printf("=============================KEY OK============================\n");
        break;
      }
    }
  }
#endif
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
