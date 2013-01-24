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

extern void mpu6050_init();
extern void ant_init();
extern void bluetooth_init();

#include <stdlib.h>
#include <stdio.h> /* For printf() */
/*---------------------------------------------------------------------------*/
PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);
/*---------------------------------------------------------------------------*/

/*
 * This process is the startup process.
 * It first shows the logo
 * Like the whole dialog intrufstture.
 */
PROCESS_THREAD(system_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  halLcdPrintXY("iWatch", 20, 70, WIDE_TEXT | HIGH_TEXT);  
  // give time to starts
  etimer_set(&et, CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  //mpu6050_init();

  lcd_init();
  ant_init();
  bluetooth_init();

  //etimer_adjust(&et, CLOCK_SECOND * 10);
  //etimer_restart(&et);
  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);
    {
    printf("Key Changed %d, %d, %d, %d\n", 
           button_sensor.value(0),
           button_sensor.value(1),
           button_sensor.value(2),
           button_sensor.value(3)
           );
    }
  }
  
  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
