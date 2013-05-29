#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include <stdio.h>

uint8_t month, now_month, day, now_day;
uint16_t year, now_year;

static const char *month_name[] = {
  "JANUARY","FEBRUARY","MARCH","APRIL", "MAY","JUNE","JULY","AUGUST","SEPTEMBER","OCTOBER","NOVEMBER","DECEMBER"
};

static const char* week_name[] = {
  "SU", "MO", "TU", "WE", "TH", "FR", "SA"
};

static void OnDraw(tContext *pContext)
{
  char buf[20];
  // clear screen
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  // draw table title
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  const tRectangle rect = {0, 38, 255, 52};
  GrRectFill(pContext, &rect);

  // draw the title bar
  GrContextFontSet(pContext, &g_sFontNova16);
  sprintf(buf, "%s %d", month_name[month - 1], year);
  uint8_t width = GrStringWidthGet(pContext, buf, -1);
  GrStringDraw(pContext, buf, -1, (LCD_X_SIZE - width) / 2, 20, 0);

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova13);
  for(int i = 0; i < 7; i++)
  {
    width = GrStringWidthGet(pContext, week_name[i], 3);

    GrStringDraw( pContext, week_name[i], 3, i * 20 + (20 - width) / 2, 40, 0);
    // draw line in title bar
    //GrLineDrawV(pContext, i * 20, 40, 52);
  }

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);


  // get the start point of this month
  uint8_t weekday = rtc_getweekday(year, month, 1) - 1; // use 0 as index
  uint8_t maxday = rtc_getmaxday(year, month);
  uint8_t y = 55;

  for(int day = 1; day <= maxday; day++)
  {
    sprintf(buf, "%d", day);
    width = GrStringWidthGet(pContext, buf, -1);

    uint8_t today = now_year == year && now_month == month && now_day == day;
    if (today)
    {
      const tRectangle rect = {weekday * 20 + 1, y, 20 + weekday * 20 - 1, y + 12};
      GrRectFill(pContext, &rect);
      GrContextForegroundSet(pContext, ClrBlack);
      GrContextBackgroundSet(pContext, ClrWhite);
    }
    GrStringDraw( pContext, buf, -1, weekday * 20 + (20 - width) / 2, y, 0);
    if (today)
    {
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextBackgroundSet(pContext, ClrBlack);
    }

    //GrLineDrawV(pContext, weekday * 20, 53, y + 12);

    weekday++;
    if (weekday == 7)
    {
      weekday = 0;
      y += 12;
    }
  }

  // draw the buttons
  if (month == 1)
    sprintf(buf, "%s %d", month_name[11], year - 1);
  else
    sprintf(buf, "%s %d", month_name[month - 1], year);
  window_button(pContext, KEY_ENTER, buf);

  if (month == 12)
    sprintf(buf, "%s %d", month_name[0], year + 1);
  else
    sprintf(buf, "%s %d", month_name[month+1], year);
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
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_DOWN)
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
    else if (lparam == KEY_ENTER)
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