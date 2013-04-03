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

static enum _state{
  STATE_CONFIG_HOUR,
  STATE_CONFIG_MINUTE,
  STATE_CONFIG_SECOND,

  STATE_CONFIG_READY, // the order above is assumed in the logic, don't change

  STATE_RUNNING,
  STATE_PAUSE,
  STATE_ZERO
}state;

static uint8_t times[3];
static uint32_t totaltime, lefttime;
static struct etimer timer;

extern tContext context;

static void drawTime()
{
  char data[2];

  // clear the region
  GrContextForegroundSet(&context, COLOR_BLACK);
  GrContextBackgroundSet(&context, COLOR_WHITE);
  tRectangle rect = {5, 63, 10 + 3 * 45 + 40, 94};
  GrRectFill(&context, &rect);

  for(int i = 0; i < 3; i++)
  {
    data[0] = '0' + times[i] / 10;
    data[1] = '0' + times[i] % 10;

    if (state == i)
    {
      // revert color
      GrContextForegroundSet(&context, COLOR_WHITE);
      GrContextBackgroundSet(&context, COLOR_BLACK);

      tRectangle rect = {5 + i * 45, 63, 10 + i * 45 + 40, 94};
      GrRectFill(&context, &rect);
      GrContextForegroundSet(&context, COLOR_BLACK);
      GrContextBackgroundSet(&context, COLOR_WHITE);
    }
    else
    {
      GrContextForegroundSet(&context, COLOR_WHITE);
      GrContextBackgroundSet(&context, COLOR_BLACK);
    }

    GrStringDraw(&context, data, 2, 10 + i * 45, 68, 0);
  }

  // draw the button text
  switch(state)
  {
  case STATE_CONFIG_SECOND:
  case STATE_CONFIG_MINUTE:
  case STATE_CONFIG_HOUR:
    {
      window_button(KEY_UP, "UP");
      window_button(KEY_DOWN, "DOWN");
      window_button(KEY_ENTER, "OK");
      break;
    }
  case STATE_CONFIG_READY:
    {
      window_button(KEY_UP, NULL);
      window_button(KEY_DOWN, "RESET");
      window_button(KEY_ENTER, "START");
      break;
    }
  case STATE_RUNNING:
    {
      window_button(KEY_UP, NULL);
      window_button(KEY_DOWN, "PAUSE");
      window_button(KEY_DOWN, "STOP");
      break;
    }
  }
  GrFlush(&context);
}

static int process_event(uint8_t ev, void* data)
{
  switch(state)
  {
  case STATE_CONFIG_SECOND:
  case STATE_CONFIG_MINUTE:
  case STATE_CONFIG_HOUR:
    {
      // handle up, down
      if (ev != EVENT_KEY_PRESSED)
        return 0;
      switch((uint8_t)data)
      {
      case KEY_UP:
        times[state]++;
        // limit to one day
        if ((times[state] >= 60) || (times[state] >= 24 && state == STATE_CONFIG_HOUR))
        {
          times[state] = 0;
        }
        break;
      case KEY_DOWN:
        times[state]--;
        if (times[state] == 0xff)
        {
          if (state == STATE_CONFIG_HOUR)
            times[state] = 23;
          else
            times[state] = 59;
        }
        break;
      case KEY_ENTER:
        if (state == STATE_CONFIG_HOUR)
          state = STATE_CONFIG_READY;
        else
          state--;
        break;
      }
      drawTime();

      return 1;
    }
  case STATE_CONFIG_READY:
    {
      if (ev != EVENT_KEY_PRESSED)
        return 0;

      if ((uint8_t)data == KEY_DOWN)
      {
        state = STATE_CONFIG_SECOND;
        times[0] = times[1] = times[2] = 0;
      }
      else if ((uint8_t)data == KEY_ENTER)
      {
        lefttime = totaltime = times[STATE_CONFIG_SECOND] + times[STATE_CONFIG_MINUTE] * 60
              + times[STATE_CONFIG_HOUR] * 3600;
        // setup timer every second
        etimer_set(&timer, CLOCK_SECOND);

        state = STATE_RUNNING;
      }
      drawTime();

      return 1;
    }
  case STATE_RUNNING:
    {
      if (ev == EVENT_KEY_PRESSED)
      {
        if ((uint8_t)data == KEY_DOWN)
        {
          etimer_stop(&timer);
          state = STATE_CONFIG_READY;
        }
        else if ((uint8_t)data == KEY_ENTER)
        {
          etimer_stop(&timer);
          state = STATE_CONFIG_READY;
        }
      }
      else if (ev == PROCESS_EVENT_TIMER)
      {
        lefttime--;
        times[STATE_CONFIG_SECOND] = lefttime % 60;
        times[STATE_CONFIG_MINUTE] = (lefttime/60) % 60;
        times[STATE_CONFIG_HOUR] = (lefttime/3600) % 60;
        if (lefttime == 0)
        {
          state = STATE_CONFIG_READY;
        }
        else
        {
          etimer_restart(&timer);
        }
      }
      else
        return 0;

      drawTime();
      return 1;
    }
  }

  return 0;
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
      GrContextFontSet(&context, &g_sFontCmss22);
      GrClearDisplay(&context);

      state = STATE_CONFIG_SECOND;
      times[0] = times[1] = times[2] = 0;
      drawTime();
    }
    else if (ev == EVENT_WINDOW_CLOSING)
    {
      // remove timer
      etimer_stop(&timer);
    }
    else
    {
      if (!process_event(ev, data))
        window_defproc(ev, data);
    }
  }
  PROCESS_END();
}