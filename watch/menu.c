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
  {"Bluetooth", NULL},
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
  {"Music Control", NULL},
  {"Sports Watch", NULL},
  {"Watch Setup", &menu_process},
  {NULL}
};

extern tContext context;
extern tRectangle client_clip;

#define NUM_MENU_A_PAGE 5
#define MENU_SPACE 30

static void drawMenuItem(const struct MenuItem *item, int index, int selected)
{
  if (selected)
  {
    GrContextFontSet(&context, &g_sFontCmss16b);

    // draw a rect
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrContextBackgroundSet(&context, COLOR_BLACK);
  }
  else
  {
    GrContextFontSet(&context, &g_sFontCmss16);

    GrContextForegroundSet(&context, COLOR_BLACK);
    GrContextBackgroundSet(&context, COLOR_WHITE);
  }

  tRectangle rect = {10, 16 + index * MENU_SPACE, 134, 9 + (index + 1) * MENU_SPACE};
  GrRectFill(&context, &rect);

  GrContextForegroundSet(&context, !selected);
  GrContextBackgroundSet(&context, selected);
  GrStringDraw(&context, item->name, -1, 32, 16 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
}

static void drawMenu(const struct MenuItem *item, int startIndex, int selected)
{
  if (startIndex > 0)
  {
     // draw some grey area means something in the up
  }
  item += startIndex;

  for(int i = 0; i < NUM_MENU_A_PAGE; i++)
  {
    if (item->name == NULL)
      break;

    drawMenuItem(item, i, selected == startIndex + i);
    item++;
  }

  if (item->name != NULL)
  {
    // there is something more
  }
}

static const struct MenuItem *Items;
static uint8_t currentTop, current;

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
      GrContextForegroundSet(&context, COLOR_BLACK);
      GrRectFill(&context, &client_clip);

      drawMenu(Items, currentTop, current);
      GrFlush(&context);
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

          drawMenu(Items, currentTop, current);
          GrFlush(&context);
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

          drawMenu(Items, currentTop, current);
          GrFlush(&context);
        }
      }
      else if (lparam == KEY_ENTER)
      {
        if (Items[current].handler)
        {
          window_open(Items[current].handler, NULL);
        }
      }
    }
    else
    {
     return 0;
    }

    return 1;
}