#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include <stdio.h>

uint8_t month, now_month, day, now_day;
uint16_t year, now_year;

static void OnDraw(tContext *pContext)
{
  char buf[20];
  // clear screen
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &fullscreen_clip);

  // draw table title
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  const tRectangle rect = {0, 28, 255, 42};
  GrRectFill(pContext, &rect);

  // draw the title bar
  GrContextFontSet(pContext, &g_sFontNova16b);
  sprintf(buf, "%s %d", month_name[month - 1], year);
  GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE / 2, 15, 0);

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova16b);
  for(int i = 0; i < 7; i++)
  {
    GrStringDrawCentered( pContext, week_shortname[i], 3, i * 20 + 10, 35, 0);
    // draw line in title bar
    // GrLineDrawV(pContext, i * 20, 40, 42);
  }

  GrContextFontSet(pContext, &g_sFontNova16);
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);

  // get the start point of this month
  uint8_t weekday = rtc_getweekday(year, month, 1) - 1; // use 0 as index
  uint8_t maxday = rtc_getmaxday(year, month);
  uint8_t y = 50;

  for(int day = 1; day <= maxday; day++)
  {
    sprintf(buf, "%d", day);

    uint8_t today = now_year == year && now_month == month && now_day == day;
    if (today)
    {
      const tRectangle rect = {weekday * 20 + 1, y - 7, 20 + weekday * 20 - 1, y + 7};
      GrRectFill(pContext, &rect);
      GrContextForegroundSet(pContext, ClrBlack);
      GrContextBackgroundSet(pContext, ClrWhite);
    }
    GrStringDrawCentered( pContext, buf, -1, weekday * 20 + 10, y, 0);
    if (today)
    {
      const tRectangle rect2 = {weekday * 20 + 16, y - 5, weekday * 20 + 17, y - 4};
      GrRectFill(pContext, &rect2);

      GrContextForegroundSet(pContext, ClrWhite);
      GrContextBackgroundSet(pContext, ClrBlack);
    }

    if (weekday != 6)
      GrLineDrawV(pContext, (weekday + 1 ) * 20, 40, y + 7);

    weekday++;
    if (weekday == 7)
    {
      GrLineDrawH(pContext, 0, LCD_X_SIZE, y + 8);

      weekday = 0;
      y += 17;
    }
  }

  GrLineDrawH(pContext, 0, weekday * 20, y + 8);

  // draw the buttons
  if (month == 1)
    sprintf(buf, "%s %d", month_shortname[11], year - 1);
  else
    sprintf(buf, "%s %d", month_shortname[month - 2], year);
  window_button(pContext, KEY_ENTER, buf);

  if (month == 12)
    sprintf(buf, "%s %d", month_shortname[0], year + 1);
  else
    sprintf(buf, "%s %d", month_shortname[month], year);
  window_button(pContext, KEY_DOWN, buf);
}


uint8_t calendar_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readdate(&year, &month, &day, NULL);
    now_month = month;
    now_year = year;
    now_day = day;
    return 0x80;
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_ENTER)
    {
      if (month == 1)
      {
        month = 12;
        year--;
      }
      else
      {
        month--;
      }
      window_invalid(NULL);
    }
    else if (lparam == KEY_DOWN)
    {
      if (month == 12)
      {
        month = 1;
        year++;
      }
      else
      {
        month++;
      }
      window_invalid(NULL);
    }
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    OnDraw((tContext*)rparam);
  }
  else
  {
    return 0;
  }

  return 1;
}