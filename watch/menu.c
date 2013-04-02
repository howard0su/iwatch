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
PROCESS(menu_process, "Menu Window");

struct MenuItem
{
  char *name;
  struct process* handler;
};

static struct MenuItem SetupMenu[] =
{
  {"Bluetooth", NULL},
  {"ANT+", NULL},
  {NULL}
};

static struct MenuItem MainMenu[] =
{
  {"Today's Activity", NULL},
  {"Analog Watch", &analogclock_process},
  {"Digital Watch", &digitclock_process},
  {"World Clock", NULL},
  {"Calendar", NULL},
  {"Stop Watch", NULL},
  {"Countdown Timer", NULL},
  {"Music Control", NULL},
  {"Sports Watch", NULL},
  {"Watch Setup", &menu_process},
  {NULL}
};

extern tContext context;

#define NUM_MENU_A_PAGE 5
#define MENU_SPACE 30

static void drawMenuItem(struct MenuItem *item, int index, int selected)
{
  if (selected)
  {
    // draw a rect
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrContextBackgroundSet(&context, COLOR_BLACK);
  }
  else
  {
    GrContextForegroundSet(&context, COLOR_BLACK);
    GrContextBackgroundSet(&context, COLOR_WHITE);
  }

  tRectangle rect = {10, 16 + index * MENU_SPACE, 134, 10 + (index + 1) * MENU_SPACE};
  GrRectFill(&context, &rect);

  GrContextForegroundSet(&context, !selected);
  GrContextBackgroundSet(&context, selected);
  GrStringDraw(&context, item->name, -1, 32, 16 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
}

static void drawMenu(struct MenuItem *item, int startIndex)
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

    drawMenuItem(item, i, 0);
    item++;
  }

  if (item->name != NULL)
  {
    // there is something more
  }
}

static struct MenuItem *Items;
static uint8_t currentTop, current;
PROCESS_THREAD(menu_process, ev, data)
{
  PROCESS_BEGIN();

  while(1)
  {
    PROCESS_WAIT_EVENT();

    if (ev == EVENT_WINDOW_CREATED)
    {
      Items = (struct MenuItem*)data;
      if (Items == NULL)
      {
        Items = MainMenu;
      }
      current = currentTop = 0;
      GrContextInit(&context, &g_memlcd_Driver);
      GrContextFontSet(&context, &g_sFontCm12);

      GrClearDisplay(&context);

      drawMenu(Items, currentTop);
      drawMenuItem(&Items[0], current - currentTop, 1);
      GrFlush(&context);
    }
    else if (ev == EVENT_KEY_PRESSED)
    {
      if ((uint8_t)data == KEY_UP)
      {
        if (current > 0)
        {
          current--;
          if (currentTop > current)
          {
            currentTop--;
            drawMenu(Items, currentTop);
          }
          else
          {
            // deselect the previous one
            drawMenuItem(&Items[current+1], current + 1 - currentTop, 0);
          }

          drawMenuItem(&Items[current], current - currentTop, 1);
          GrFlush(&context);
        }
      }
      else if ((uint8_t)data == KEY_DOWN)
      {
        if (Items[current+1].name != NULL)
        {
          current++;
          if (currentTop + NUM_MENU_A_PAGE <= current)
          {
            currentTop++;
            drawMenu(Items, currentTop);
          }
          else
          {
            drawMenuItem(&Items[current - 1], current - 1 - currentTop, 0);
          }
          drawMenuItem(&Items[current], current - currentTop, 1);
          GrFlush(&context);
        }
      }
      else if ((uint8_t)data == KEY_ENTER)
      {
        if (Items[current].handler)
        {
          window_open(Items[current].handler, NULL);
        }
      }
    }
    else
    {
      window_defproc(ev, data);
    }
  }

  PROCESS_END();
}