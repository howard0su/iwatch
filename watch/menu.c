/****************************************************************
*  Description: Implementation for main Menu dialog
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
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include <stdio.h>
/*
* This implement the menu
*/

#define DATA_DATE 0xF1
#define DATA_TIME 0xF2
#define DATA_ANT  0xF3
#define DATA_BT   0xF4

struct MenuItem
{
  char icon;
  const char *name;
  windowproc handler;
};

static const struct MenuItem SetupMenu[] =
{
  {DATA_DATE, "Date", &configdate_process},
  {DATA_TIME, "Time", &configtime_process},
  {DATA_BT, "Bluetooth", &btconfig_process},
  {0, "Upgrade Firmware", &upgrade_process},
  {0, "Self-test", &selftest_process},
  {-1, NULL, NULL}
};

static const struct MenuItem MainMenu[] =
{
  {'a', "Today's Activity", &today_process},
  {'b', "Analog Watch", &analogclock_process},
  {'c', "Digital Watch", &digitclock_process},
  {'d', "World Clock", &worldclock_process},
  {'e', "Calendar", &calendar_process},
  {'f', "Stop Watch", &stopwatch_process},
  {'g', "Countdown Timer", &countdown_process},
  {'k', "Music Control", &control_process},
  {'h', "Sports Watch", &sporttype_process},
  {'l', "Watch Setup", &menu_process},
  {0, NULL, NULL}
};

#define NUM_MENU_A_PAGE 5
#define MENU_SPACE 30

static void drawMenuItem(tContext *pContext, const struct MenuItem *item, int index, int selected)
{
  if (selected)
  {
    // draw a rect
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextBackgroundSet(pContext, ClrBlack);
  }
  else
  {
    GrContextForegroundSet(pContext, ClrBlack);
    GrContextBackgroundSet(pContext, ClrWhite);
  }

  tRectangle rect = {8, 17 + index * MENU_SPACE, 136, 9 + (index + 1) * MENU_SPACE};
  GrRectFillRound(pContext, &rect, 2);

  GrContextForegroundSet(pContext, !selected);
  GrContextBackgroundSet(pContext, selected);
  if (item->icon > 0)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
    GrStringDraw(pContext, &item->icon, 1, 10, 14 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
  }

  GrContextFontSet(pContext, &g_sFontBaby16);
  if (item->icon > 0)
  {
    GrStringDraw(pContext, item->name, -1, 32, 16 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
  }
  else
  {
    char buf[20];
    int width;
    // <= 0
    GrStringDraw(pContext, item->name, -1, 12, 16 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);

    switch(item->icon)
    {
      case 0:
        return;
      case DATA_DATE:
      {
        uint8_t month, day;
        rtc_readdate(NULL, &month, &day, NULL);
        sprintf(buf, "%s %d", month_shortname[month], day);
        break;
      }
      case DATA_TIME:
      {
        uint8_t hour, minute;
        rtc_readtime(&hour, &minute, NULL);
        sprintf(buf, "%02d:%02d", hour, minute);
        break;
      }
      case DATA_BT:
      sprintf(buf, "%s", "ON");
      break;
      case DATA_ANT:
      sprintf(buf, "%s", "OFF");
      break;
    }
    width = GrStringWidthGet(pContext, buf, -1);
    GrStringDraw(pContext, buf, -1, LCD_X_SIZE - 12 - width, 16 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
  }
}

static const struct MenuItem *Items;
static uint8_t currentTop, current;
static uint8_t menuLength;

static void OnDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  struct MenuItem const * item = Items;

  if (currentTop > 0)
  {
    // draw some grey area means something in the up
  }
  item += currentTop;

  for(int i = 0; i < NUM_MENU_A_PAGE; i++)
  {
    if (item->name == NULL)
      break;

    drawMenuItem(pContext, item, i, current == currentTop + i);
    item++;
  }

  if (item->name != NULL)
  {
    // there is something more
  }
}

static void getMenuLength()
{
  menuLength = 0;
  while(Items[menuLength].name != NULL)
    menuLength++;
}

uint8_t menu_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  static struct etimer timer;
  switch(ev)
  {
    case EVENT_WINDOW_CREATED:
    {
      Items = (struct MenuItem*)rparam;
      if (Items == NULL)
      {
        Items = MainMenu;
      }
      if (rparam == (void*)1)
      {
        Items = SetupMenu;
      }
      getMenuLength();

      current = currentTop = 0;
      etimer_set(&timer, CLOCK_SECOND * 30);
      break;
    }
    case PROCESS_EVENT_TIMER:
    {
      if (rparam == &timer)
      {
        // check analog or digit
        if (!window_readconfig()->default_clock)
          window_open(&analogclock_process, NULL);
        else
          window_open(&digitclock_process, NULL);
      }
      break;
    }
    case EVENT_WINDOW_PAINT:
    {
      OnDraw((tContext*)rparam);
      return 1;
    }
    case EVENT_KEY_PRESSED:
    {
      etimer_set(&timer, CLOCK_SECOND * 30);
      if (lparam == KEY_UP)
      {
        if (current > 0)
        {
          current--;
          if (currentTop > current)
          {
            currentTop--;
          }
        }
        else
        {
          current = menuLength - 1;
          currentTop = current - NUM_MENU_A_PAGE + 1;
        }

      ///TODO: optimize this
        window_invalid(NULL);
      }
      else if (lparam == KEY_DOWN)
      {
        if (Items[current+1].name != NULL)
        {
          current++;
          if (currentTop + NUM_MENU_A_PAGE <= current)
          {
            currentTop++;
          }
        }
        else
        {
          current = currentTop = 0;
        }
      ///TODO: optimize this
        window_invalid(NULL);
      }
      else if (lparam == KEY_ENTER)
      {
        if (Items[current].handler)
        {
          if (Items[current].handler == &menu_process)
          {
            if (current == 9)
            {
              Items = SetupMenu;
              getMenuLength();
              current = currentTop = 0;
              window_invalid(NULL);
            }
          }
          else
          {
            window_open(Items[current].handler, NULL);
          }
        }
      }
      break;
    }
    case EVENT_EXIT_PRESSED:
    {
      if (Items != MainMenu)
      {
        Items = MainMenu;
        currentTop = 5;
        current = 9;
        window_invalid(NULL);
      }
      else
      {
        // this is main menu
        // check analog or digit
        if (!window_readconfig()->default_clock)
          window_open(&analogclock_process, NULL);
        else
          window_open(&digitclock_process, NULL);
      }
      break;
    }
    default:
      return 0;
  }

  return 1;
}