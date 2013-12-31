#include "contiki.h"
#include "window.h"
#include "math.h"
#include "grlib/grlib.h"
#include "rtc.h"
#include "memory.h"

#include <stdio.h>

#define MINUTE  d.config.t[1]
#define HOUR    d.config.t[0]

#define DAY     d.config.t[2]
#define MONTH   d.config.t[1]
#define YEAR    d.config.t[0]

#define times   d.config.t

#define state   d.config.state

#define   STATE_CONFIG_READY 10

enum _statetime{
  STATE_CONFIG_HOUR, // times[0]
  STATE_CONFIG_MINUTE, // times[1]

};

enum _statedate{
  STATE_CONFIG_YEAR,
  STATE_CONFIG_MONTH,
  STATE_CONFIG_DAY,
};

static void GrStringDrawReverse(tContext *pContext, char* pcString, long lLength, long lX, long lY, unsigned long bOpaque)
{
  int height = GrStringHeightGet(pContext);
  int width = GrStringWidthGet(pContext, pcString, lLength);

  tRectangle rect = {lX - 2, lY - 2, lX + width + 2, lY + height + 2};
  GrContextForegroundSet(pContext, ClrWhite);
  GrRectFillRound(pContext, &rect, 3);
  GrContextForegroundSet(pContext, ClrBlack);

  GrStringDraw(pContext, pcString, lLength, lX, lY, bOpaque);

  GrContextForegroundSet(pContext, ClrWhite);
}

static void OnDrawTime(tContext *pContext)
{
  char buf[20];
  GrContextFontSet(pContext, &g_sFontNova38b);

  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  sprintf(buf, "%d", HOUR);
  if (state == STATE_CONFIG_HOUR)
  {
    GrStringDrawReverse(pContext, buf, -1, 10, 70, 1);
  }
  else
  {
    GrStringDraw(pContext, buf, -1, 10, 70, 0);
  }

  GrStringDraw(pContext, ":", 1, 55, 70, 0);

  sprintf(buf, "%d", MINUTE);
  if (state == STATE_CONFIG_MINUTE)
  {
    GrStringDrawReverse(pContext, buf, -1, 70, 70, 0);
  }
  else
  {
    GrStringDraw(pContext, buf, -1, 70, 70, 0);
  }

  GrContextFontSet(pContext, &g_sFontNova16b);
  // draw AM/PM
  if (HOUR > 12)
    GrStringDraw(pContext, "PM", 2, 120, 93, 0);
  else
    GrStringDraw(pContext, "AM", 2, 120, 93, 0);

  if (state != STATE_CONFIG_READY)
  {
    window_button(pContext, KEY_UP, "UP");
    window_button(pContext, KEY_DOWN, "DOWN");
    window_button(pContext, KEY_ENTER, "OK");
  }
}

static void OnDrawDate(tContext *pContext)
{
  char buf[20];
  GrContextFontSet(pContext, &g_sFontNova28b);

  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  sprintf(buf, "%4d", 2000 + YEAR);
  if (state == STATE_CONFIG_YEAR)
  {
    GrStringDrawReverse(pContext, buf, -1, 40, 100, 0);
  }
  else
  {
    GrStringDraw(pContext, buf, -1, 40, 100, 0);
  }

  sprintf(buf, "%s", month_shortname[MONTH - 1]);
  if (state == STATE_CONFIG_MONTH)
  {
    GrStringDrawReverse(pContext, buf, -1, 25, 60, 0);
  }
  else
  {
    GrStringDraw(pContext, buf, -1, 25, 60, 0);
  }

  sprintf(buf, "%d", DAY);
  if (state == STATE_CONFIG_DAY)
  {
    GrStringDrawReverse(pContext, buf, -1, 90, 60, 0);
  }
  else
  {
    GrStringDraw(pContext, buf, -1, 90, 60, 0);
  }

  if (state != STATE_CONFIG_READY)
  {
    window_button(pContext, KEY_UP, "UP");
    window_button(pContext, KEY_DOWN, "DOWN");
    window_button(pContext, KEY_ENTER, "OK");
  }
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
      rtc_settime(HOUR, MINUTE, 0);
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
    if ((times[state] >= rtc_getmaxday(2000 + YEAR, MONTH)) || (times[state] >= 12 && state == STATE_CONFIG_MONTH))
    {
      times[state] = 1;
    }
    break;
  case KEY_DOWN:
    times[state]--;
    if (times[state] == 0)
    {
      if (state == STATE_CONFIG_MONTH) // times[1]
        MONTH = 12;
      else if (state == STATE_CONFIG_DAY)
        DAY = rtc_getmaxday(2000 + YEAR, MONTH);
    }
    else if (times[state] == 0xff)
    {
      times[state] = 99;
    }
    break;
  case KEY_ENTER:
    if (state == STATE_CONFIG_YEAR)
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
      state = STATE_CONFIG_DAY;
      rtc_readdate(&data, &MONTH, &DAY, NULL);
      YEAR = data - 2000;
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDrawDate((tContext*)rparam);
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
      state = STATE_CONFIG_MINUTE;
      rtc_readtime(&HOUR, &MINUTE, 0);
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDrawTime((tContext*)rparam);
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


static void OnDrawLevel(tContext *pContext, int level)
{
    // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  window_volume(pContext, 30, 100, 8, level);

  window_button(pContext, KEY_UP, "+");
  window_button(pContext, KEY_DOWN, "-");
  window_button(pContext, KEY_ENTER, "OK");
}

uint8_t configlight_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      state = window_readconfig()->light_level;
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDrawLevel((tContext*)rparam, window_readconfig()->light_level);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      int level = window_readconfig()->light_level;
      switch(lparam)
      {
        case KEY_UP:
        if (level < 8)
          level++;
        break;
        case KEY_DOWN:
        if (level > 0)
          level--;
        break;
        case KEY_ENTER:
          window_writeconfig();
          window_close();
          return 1;
      }
      
      window_readconfig()->light_level = level;
      window_invalid(NULL);
      break;
    }
    case EVENT_EXIT_PRESSED:
      //revert back light level
      window_readconfig()->light_level = state;
      window_writeconfig();
      window_close();
      break;
  default:
    return 0;
  }

  return 1;
}

uint8_t configvol_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      state = window_readconfig()->volume_level;
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDrawLevel((tContext*)rparam, state);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      switch(lparam)
      {
        case KEY_UP:
        if (state < 8)
          state++;
        break;
        case KEY_DOWN:
        if (state > 0)
          state--;
        break;
        case KEY_ENTER:
          window_readconfig()->volume_level = state;
          window_writeconfig();
          window_close();
          return 1;
      }
      window_invalid(NULL);
      break;
    }
  default:
    return 0;
  }

  return 1;
}