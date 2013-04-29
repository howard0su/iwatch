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

/*
* This implement the menu
*/

struct MenuItem
{
  char *name;
  windowproc handler;
};

static const struct MenuItem SetupMenu[] =
{
  {"Date", configdate_process},
  {"Time", configtime_process},
  {"Bluetooth", btconfig_process},
  {"ANT+", NULL},
  {NULL}
};

static const struct MenuItem MainMenu[] =
{
  {"Today's Activity", NULL},
  {"Analog Watch", &analogclock_process},
  {"Digital Watch", &digitclock_process},
  {"World Clock", NULL},
  {"Calendar", NULL},
  {"Stop Watch", NULL},
  {"Countdown Timer", &countdown_process},
  {"Music Control", &control_process},
  {"Sports Watch", NULL},
  {"Watch Setup", &menu_process},
  {NULL}
};

#define NUM_MENU_A_PAGE 5
#define MENU_SPACE 30

static void drawMenuItem(tContext *pContext, const struct MenuItem *item, int index, int selected)
{
  if (selected)
  {
    GrContextFontSet(pContext, &g_sFontNova12b);

    // draw a rect
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextBackgroundSet(pContext, ClrBlack);
  }
  else
  {
    GrContextFontSet(pContext, &g_sFontNova12);

    GrContextForegroundSet(pContext, ClrBlack);
    GrContextBackgroundSet(pContext, ClrWhite);
  }

  tRectangle rect = {10, 17 + index * MENU_SPACE, 134, 9 + (index + 1) * MENU_SPACE};
  GrRectFill(pContext, &rect);

  GrContextForegroundSet(pContext, !selected);
  GrContextBackgroundSet(pContext, selected);
  GrStringDraw(pContext, item->name, -1, 32, 17 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
}

static const struct MenuItem *Items;
static uint8_t currentTop, current;

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

uint8_t menu_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    Items = (struct MenuItem*)rparam;
    if (Items == NULL)
    {
      Items = MainMenu;
    }
    current = currentTop = 0;
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    OnDraw((tContext*)rparam);
    return 1;
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_UP)
    {
      if (current > 0)
      {
        current--;
        if (currentTop > current)
        {
          currentTop--;
        }

        ///TODO: optimize this
        window_invalid(NULL);
      }
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

        ///TODO: optimize this
        window_invalid(NULL);
      }
    }
    else if (lparam == KEY_ENTER)
    {
      if (Items[current].handler)
      {
        if (Items[current].handler == &menu_process)
        {
          if (current == 9)
          {
            window_open(&menu_process, (void*)&SetupMenu);
          }
        }
        else
        {
          window_open(Items[current].handler, NULL);
        }
      }
    }
  }
  else
  {
    return 0;
  }

  return 1;
}