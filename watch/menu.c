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
#include "hal_lcd.h"
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

#define NUM_MENU_A_PAGE 8

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

    halLcdPrintXY(item->name,32, 32 + i * 16, 0);
    item++;
  }

  if (item->name != NULL)
  {
    // there is something more
  }
}

static void drawCurrent(int index)
{
  halLcdHLine(0, LCD_ROW, 28 + index*16, 18, STYLE_XOR);
}

static struct MenuItem *Items;
static uint8_t currentTop, current;
PROCESS_THREAD(menu_process, ev, data)
{
  PROCESS_BEGIN();

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
      halLcdClearScreen();

      drawMenu(Items, currentTop);
      drawCurrent(current - currentTop);
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
            drawCurrent(current + 1 - currentTop);
          }
          drawCurrent(current - currentTop);
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
            drawCurrent(current - 1 - currentTop);
          }
          drawCurrent(current - currentTop);
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