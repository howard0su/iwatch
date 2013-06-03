#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"

static uint8_t times[3]; // minute, second, 10ms
static uint8_t counter;
static enum
{
  STATE_RUNNING = 0,
  STATE_STATE1,
  STATE_STATE2, // have to be that order
  STATE_STOP,
  STATE_INIT
}state;
static uint8_t saved_times[3][3]; // saved time

static void OnDraw(tContext* pContext)
{
  // clear the screen
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  // draw the countdown time
  GrContextFontSet(pContext, &g_sFontNova38b);
  window_drawtime(pContext, 35, times, 0);

  if (state != STATE_INIT)
  {
    GrContextFontSet(pContext, &g_sFontNova16);
    // draw the stoped times
    for(int i = 0; i < state; i++)
    {
      window_drawtime(pContext, i * 20 + 90, saved_times[i], 0);
    }
  }
}

uint8_t stopwatch_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    state = STATE_INIT;
    break;
  case EVENT_WINDOW_PAINT:
    OnDraw((tContext*)rparam);
    break;
  case EVENT_TIME_CHANGED:
    {
      // 32Hz interrupt
      tRectangle rect = {0, 30, LCD_Y_SIZE, 64};
      counter+=2;
      if (counter >= 32)
        {
          times[1]++;
          counter -= 32;
        }
      times[2] = counter * 3;
      if (times[1] == 60)
        {
          times[1] = 0;
          times[0]++;
        }

      if (times[0] == 60)
        {
          // overflow
          state = STATE_STOP;
          rtc_enablechange(0);
        }
      window_invalid(&rect);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      if (lparam == KEY_ENTER)
      {
        if (state == STATE_STOP || state == STATE_INIT)
        {
          // let's start
          rtc_enablechange(TENMSECOND_CHANGE);
          state = STATE_RUNNING;
          times[0] = times[1] = times[2] = 0;
          counter = 0;
        }
        else
        {
          if (state != STATE_STOP)
          {
            saved_times[state][0] = times[0];
            saved_times[state][1] = times[1];
            saved_times[state][2] = times[2];
            state++;
          }

          if (state == STATE_STOP)
          {
            rtc_enablechange(0);
          }
        }
        window_invalid(NULL);
      }
      break;
    }
  case EVENT_WINDOW_CLOSING:
    {
      rtc_enablechange(0);
    }
  default:
    return 0;
  }

  return 1;
}