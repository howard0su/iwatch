#include "contiki.h"

#include "window.h"
#include "rtc.h"
#include <stdio.h>

static void drawItem(tContext *pContext,
                     uint8_t y,
                     const char* name,
                     uint8_t hour, uint8_t minute,
                     uint8_t today)
{
  const char *ampm;
  char buf[20];

  if (hour > 12)
  {
    hour -= 12;
    ampm = "PM";
  }
  else
    ampm = "AM";

  GrContextFontSet(pContext, (tFont*)&g_sFontNova16b);
  GrStringDraw(pContext, name, -1, 12, y + 10, 0);

  GrContextFontSet(pContext, (tFont*)&g_sFontNova16);
  sprintf(buf, "%02d:%02d %s", hour, minute, ampm);
  GrStringDraw(pContext, buf, -1, 12, y + 28, 0);

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  switch(today)
  {
  case 0:
    GrStringDraw(pContext, "today", -1, 88, y + 28, 1);
    break;
  case 1:
    GrStringDraw(pContext, "tomorrow", -1, 88, y + 28, 1);
    break;
  case 2:
    GrStringDraw(pContext, "yesterday", -1, 88, y + 28, 1);
    break;
  }
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
}

static void onDraw(tContext *pContext)
{
  // clear the screen
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);

  for(int i = 0; i < 3; i++)
  {
    int8_t hour;
    uint8_t minute;
    rtc_readtime((uint8_t*)&hour, &minute, NULL);

    uint8_t today = 0;
    int8_t offset = window_readconfig()->worldclock_offset[i];
    hour += offset;
    if (hour > 24)
    {
      today = 1;
      hour -= 24;
    }
    else if (hour < 0)
    {
      today = 2;
      hour += 24;
    }

    drawItem(pContext, 16 + i * 50,  window_readconfig()->worldclock_name[i],
             hour, minute, today);
  }
}

uint8_t worldclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_EXIT)
      window_close();
    break;
  default:
    return 0;
  }

  return 1;
}