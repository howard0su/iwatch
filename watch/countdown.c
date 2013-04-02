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
#include "math.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

PROCESS(countdown_process, "Countdown Watch Window");

enum _state{
  STATE_CONFIG_SECOND,
  STATE_CONFIG_MINUTE,
  STATE_CONFIG_HOUR,

  STATE_NO_CONFIG,

  STATE_RUNNING,
  STATE_ZERO
}state;

static uint8_t times;
static uint32_t totaltime;
static struct etimer timer;

static int process_event(uint8_t ev, void* data)
{
  switch(state)
  {
  case STATE_CONFIG_SECOND:
  case STATE_CONFIG_MINUTE:
  case STATE_CONFIG_HOUR:
    {
      // handle up, down

    }
  }
}

PROCESS_THREAD(countdown_process, ev, data)
{
  PROCESS_BEGIN();
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == EVENT_WINDOW_CREATED)
    {
      // initialize state

    }
    else
    {
      window_defproc(ev, data);
    }
  }
  PROCESS_END();
}