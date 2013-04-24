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

static void drawTime(tContext *pContext)
{
  char data[2];

  // initialize state
  GrContextFontSet(pContext, &g_sFontNova30b);

  // clear the region
  GrContextForegroundSet(pContext, COLOR_BLACK);
  GrContextBackgroundSet(pContext, COLOR_WHITE);
  GrRectFill(pContext, &client_clip);

  for(int i = 0; i < 3; i++)
  {
    data[0] = '0' + times[i] / 10;
    data[1] = '0' + times[i] % 10;

    if (state == i)
    {
      // revert color
      GrContextForegroundSet(pContext, COLOR_WHITE);
      GrContextBackgroundSet(pContext, COLOR_BLACK);

      tRectangle rect = {5 + i * 45, 63, 10 + i * 45 + 35, 94};
      GrRectFill(pContext, &rect);
      GrContextForegroundSet(pContext, COLOR_BLACK);
      GrContextBackgroundSet(pContext, COLOR_WHITE);
    }
    else
    {
      GrContextForegroundSet(pContext, COLOR_WHITE);
      GrContextBackgroundSet(pContext, COLOR_BLACK);
    }

    GrStringDraw(pContext, data, 2, 10 + i * 45, 68, 0);

    if (i != 2)
    {
      GrContextForegroundSet(pContext, COLOR_WHITE);
      GrContextBackgroundSet(pContext, COLOR_BLACK);
      GrStringDraw(pContext, ":", 1, 45 + i * 45, 63, 0);
    }
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

      // display progress bar
      if (totaltime != lefttime)
        window_progress(100, 100 - (uint8_t)(lefttime * 100 / totaltime));

      break;
    }
  case STATE_RUNNING:
    {
      window_button(KEY_UP, NULL);
      window_button(KEY_DOWN, "PAUSE");
      window_button(KEY_ENTER, "STOP");

      // display progress bar
      window_progress(100, 100 - (uint8_t)(lefttime * 100 / totaltime));

      break;
    }
  }
}

static int process_event(uint8_t ev, uint16_t data)
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
      switch(data)
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
      window_invalid(NULL);

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
      else if (data == KEY_ENTER)
      {
        lefttime = totaltime = times[STATE_CONFIG_SECOND] + times[STATE_CONFIG_MINUTE] * 60
              + times[STATE_CONFIG_HOUR] * 3600;
        // setup timer every second
        window_timer(CLOCK_SECOND);

        state = STATE_RUNNING;
      }
      window_invalid(NULL);

      return 1;
    }
  case STATE_RUNNING:
    {
      if (ev == EVENT_KEY_PRESSED)
      {
        if (data == KEY_DOWN)
        {
          // pause
          window_timer(0);
          state = STATE_CONFIG_READY;
        }
        else if (data == KEY_ENTER)
        {
          // stop
          totaltime = lefttime;
          window_timer(0);
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
          window_timer(CLOCK_SECOND);
        }
      }
      else
        return 0;

      window_invalid(NULL);
      return 1;
    }
  }

  return 0;
}

uint8_t countdown_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    state = STATE_CONFIG_SECOND;
    times[0] = times[1] = times[2] = 0;
  }
  else if (ev == EVENT_WINDOW_CLOSING)
  {
    // remove timer
    window_timer(0);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
     drawTime((tContext*)rparam);
  }
  else
  {
    return process_event(ev, lparam);
  }

  return 1;
}