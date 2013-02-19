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

#include <stdlib.h>
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);
/*---------------------------------------------------------------------------*/

struct process* ui_process = NULL;

void window_init()
{
  return;
}

static tContext context;

#define MAX_DIALOG_DEPTH 5
#if 0
static struct process *dialogStack[MAX_DIALOG_DEPTH];
static uint8_t dialogStackPtr = 0;

void window_showdialog(struct process* dialog, void* data)
{
  process_start(dialog, data);
  dialogStack[dialogStackPtr++] = ui_process;
  ui_process = dialog;
  process_post(dialog, EVENT_WINDOW_CREATED, NULL);
}
#endif

void window_open(struct process* dialog, void* data)
{
  process_post(ui_process, EVENT_WINDOW_CLOSING, NULL);
  process_exit(ui_process);

  process_start(dialog, data);
  ui_process = dialog;
}

static uint8_t bt_status;

void window_defproc(process_event_t ev, process_data_t data)
{
  switch(ev)
  {
  case EVENT_BT_STATUS:
    {
      bt_status = (uint8_t)data;
      if (bt_status & BIT0) GrStringDraw(&context, "B", 1, 90, 2, 0);
      else GrStringDraw(&context, " ", 1, 90, 2, 0);
      if (bt_status & BIT1) GrStringDraw(&context, "=", 1, 102, 2, 0);
      else GrStringDraw(&context, " ", 1, 102, 2, 0);
      GrFlush(&context);
      break;
    }
  case PROCESS_EVENT_INIT:
    {
      process_post(PROCESS_CURRENT(), EVENT_WINDOW_CREATED, data);
      process_post(PROCESS_CURRENT(), EVENT_BT_STATUS, (void*)bt_status);
      break;
    }
  }
  return;
}

/*
* This process is the startup process.
* It first shows the logo
* Like the whole dialog intrufstture.
*/
PROCESS_THREAD(system_process, ev, data)
{
  PROCESS_BEGIN();
  ui_process = &analogclock_process;

  rtc_init();
  SENSORS_ACTIVATE(button_sensor);

  memlcd_DriverInit();
  {
    GrContextInit(&context, &g_memlcd_Driver);
    GrContextFontSet(&context, &g_sFontCm32);
    GrClearDisplay(&context);
    GrStringDraw(&context, "iWatch", -1, 20, 70, 0);
    GrFlush(&context);
    GrContextFontSet(&context, &g_sFontCm12);
  }
  // give time to starts
  //mpu6050_init();
  ant_init();
  bluetooth_init();

  process_start(ui_process, NULL);
  print_stats();
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == sensors_event)
    {
      printf("Key Changed %d, %d, %d, %d\n",
             button_sensor.value(0),
             button_sensor.value(1),
             button_sensor.value(2),
             button_sensor.value(3)
               );
      if (button_sensor.value(0) == 1)
      {
        if (ui_process != &menu_process)
        {
          window_open(&menu_process, NULL);
        }
        else
        {
          process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_UP);
        }
      }
      if (button_sensor.value(1) == 1)
      {
        process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_DOWN);
      }
      if (button_sensor.value(2) == 1)
      {
        process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_ENTER);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
