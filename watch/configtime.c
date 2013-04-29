#include "contiki.h"
#include "window.h"
#include "math.h"
#include "grlib/grlib.h"
#include "rtc.h"

static uint8_t times[3];
//       times[0], times[1], times[2];
//reuse times[0], times[1],  times[2];

#define SECOND times[2]
#define MINUTE times[1]
#define HOUR   times[0]

#define DAY times[2]
#define MONTH times[1]
#define YEAR   times[0]



static enum _state{
  STATE_CONFIG_HOUR, // times[0]
  STATE_CONFIG_MINUTE, // times[1]
  STATE_CONFIG_SECOND, // date

  STATE_CONFIG_READY, // the order above is assumed in the logic, don't change
}state;


static void OnDraw(tContext *pContext)
{
  GrContextFontSet(pContext, &g_sFontNova28b);

  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  window_drawtime(pContext, 0, times[0], times[1], times[2], 1 << state);

  if (state != STATE_CONFIG_READY)
  {
    window_button(pContext, KEY_UP, "UP");
    window_button(pContext, KEY_DOWN, "DOWN");
    window_button(pContext, KEY_ENTER, "OK");
  }
  window_button(pContext, KEY_EXIT, "ABORT");
}

static int process_key_time(uint8_t key)
{
  if (key == KEY_EXIT)
  {
    window_close();
    return 1;
  }

  if (state == STATE_CONFIG_READY)
    return 0;

  switch(key)
  {
  case KEY_UP:
    times[state]++;
    // limit to one times[2]
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
    {
      state = STATE_CONFIG_READY;
      rtc_settime(HOUR, MINUTE, SECOND);
    }
    else
    {
      state--;
    }
    break;
  case KEY_EXIT:
    {
      window_close();
    }
  }
  window_invalid(NULL);

  return 1;
}

static int process_key_date(uint8_t key)
{
  if (key == KEY_EXIT)
  {
    window_close();
    return 1;
  }

  if (state == STATE_CONFIG_READY)
    return 0;

  switch(key)
  {
  case KEY_UP:
    times[state]++;
    if ((times[state] >= rtc_getmaxday(2000 + YEAR, MONTH)) || (times[state] >= 12 && state == STATE_CONFIG_HOUR))
    {
      times[state] = 1;
    }
    break;
  case KEY_DOWN:
    times[state]--;
    if (times[state] == 0)
    {
      if (state == STATE_CONFIG_MINUTE) // times[1]
        MONTH = 12;
      else if (state == STATE_CONFIG_SECOND)
        DAY = rtc_getmaxday(2000 + YEAR, MONTH);
    }
    else if (times[state] == 0xff)
    {
      times[state] = 99;
    }
    break;
  case KEY_ENTER:
    if (state == STATE_CONFIG_HOUR)
    {
      state = STATE_CONFIG_READY;
      rtc_setdate(2000 + YEAR, MONTH, DAY);
    }
    else
    {
      state--;
      if (DAY > rtc_getmaxday(2000 + YEAR, MONTH))
        DAY = rtc_getmaxday(2000 + YEAR, MONTH);
    }
    break;
  case KEY_EXIT:
    {
      window_close();
    }
  }
  window_invalid(NULL);

  return 1;
}

uint8_t configdate_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      uint16_t data;
      state = STATE_CONFIG_SECOND;
      rtc_readdate(&data, &MONTH, &DAY, NULL);
      YEAR = data - 2000;
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDraw((tContext*)rparam);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      process_key_date((uint8_t)lparam);
    }
  default:
    return 0;
  }

  return 1;
}

uint8_t configtime_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      state = STATE_CONFIG_SECOND;
      rtc_readtime(&HOUR, &MINUTE, &SECOND);
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDraw((tContext*)rparam);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      process_key_time((uint8_t)lparam);
    }
  default:
    return 0;
  }

  return 1;
}