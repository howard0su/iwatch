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

static struct MenuItem
{
  char *name;
  struct process* handler;
}MainMenu[] =
{
  {"Analog Watch", &analogclock_process},
  {"Digit Watch", NULL},
  {"Stop Watch", NULL},
  {"Bluetooth", NULL},
  {"ANT+", NULL},
  {"Stop Watch", NULL},
  {NULL}
};

static tContext context;


#define NUM_MENU_A_PAGE 5

static void drawMenu(struct MenuItem *item, int startIndex)
{
  if (startIndex > 0)
  {
     // draw some grey area means something in the up
  }
  GrContextForegroundSet(&context, 0);
  GrContextBackgroundSet(&context, 1);

  item += startIndex;

  for(int i = 0; i < NUM_MENU_A_PAGE; i++)
  {
    if (item->name == NULL)
      break;

    GrStringDraw(&context, item->name, -1, 32, 14 + i * 20, 1);
    item++;
  }

  if (item->name != NULL)
  {
    // there is something more
  }

  GrFlush(&context);
}

static void drawCurrent(struct MenuItem *item, int index, int selected)
{
  GrContextForegroundSet(&context, selected);
  GrContextBackgroundSet(&context, !selected);
  GrStringDraw(&context, item[index].name, -1, 32, 14 + index * 20, 1);
  GrFlush(&context);
}

static struct MenuItem *Items;
static uint8_t currentTop, current;
PROCESS_THREAD(menu_process, ev, data)
{
  PROCESS_BEGIN();

  GrContextInit(&context, &g_memlcd_Driver);
  GrContextFontSet(&context, &g_sFontCm16b);

  while(1)
  {
    if (ev == EVENT_WINDOW_CREATED)
    {
      Items = (struct MenuItem*)data;
      if (Items == NULL)
      {
        Items = MainMenu;
      }
      current = currentTop = 0;
      GrClearDisplay(&context);

      drawMenu(Items, currentTop);
      drawCurrent(Items, current - currentTop, 1);
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
            drawCurrent(Items, current + 1 - currentTop, 0);
          }
          drawCurrent(Items, current - currentTop, 1);
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
            drawCurrent(Items, current - 1 - currentTop, 0);
          }
          drawCurrent(Items, current - currentTop, 1);
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

    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}