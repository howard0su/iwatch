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
#include "lcd.h"
#include "hal_lcd.h"
#include "rtc.h"
#include "window.h"

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

#define MAX_DIALOG_DEPTH 5
static struct process *dialogStack[MAX_DIALOG_DEPTH];
static uint8_t dialogStackPtr = 0;

void window_showdialog(struct process* dialog)
{
  process_start(dialog, NULL);
  dialogStack[dialogStackPtr++] = ui_process;
  ui_process = dialog;
  process_post(dialog, EVENT_WINDOW_CREATED, NULL);
}

void window_open(struct process* dialog)
{
  process_post(dialog, EVENT_WINDOW_CLOSING, NULL);
  process_exit(dialog);

  process_start(dialog, NULL);
  ui_process = dialog;
  process_post(dialog, EVENT_WINDOW_CREATED, NULL);
}

void window_defproc(process_event_t ev, process_data_t data)
{
  switch(ev)
  {
  case EVENT_BT_STATUS:
    {
      int i = (int)data;
      if (i & BIT0) halLcdPrintXY("B", 90, 2, 0);
               else halLcdPrintXY(" ", 90, 2, 0);
      if (i & BIT1) halLcdPrintXY("=", 98, 2, 0);
               else halLcdPrintXY(" ", 98, 2, 0);
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
  static struct etimer et;

  PROCESS_BEGIN();
  ui_process = &analogclock_process;

  rtc_init();
  SENSORS_ACTIVATE(button_sensor);

  lcd_init();
  halLcdPrintXY("iWatch", 20, 70, WIDE_TEXT | HIGH_TEXT);
  // give time to starts
  //mpu6050_init();
  ant_init();
  bluetooth_init();

  etimer_set(&et, CLOCK_SECOND * 5);
  process_start(ui_process, NULL);
  print_stats();
  etimer_restart(&et);
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
    }
    else if (ev == PROCESS_EVENT_TIMER)
    {
      print_stats();
      etimer_restart(&et);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
