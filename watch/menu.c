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
#include "btstack/bluetooth.h"
#include <stdio.h>
#include <string.h>

#include "test.h"

/*
* This implement the menu
*/

#define DATA_DATE 0xF1
#define DATA_TIME 0xF2
#define DATA_ANT  0xF3
#define DATA_BT   0xF4
#define DATA_VERSION 0xF5
#define DATA_BTADDR 0xF6
#define DATA_LEGAL 0xF7
#define NO_DATA   0xFF

struct MenuItem
{
  unsigned char icon;
  const char *name;
  windowproc handler;
};

static const struct MenuItem SetupMenu[] =
{
  {DATA_DATE, "Date", &configdate_process},
  {DATA_TIME, "Time", &configtime_process},
  {DATA_BT, "Bluetooth", &btconfig_process},
  {NO_DATA, "Upgrade Firmware", &upgrade_process},
//  {NO_DATA, "Self-test", &selftest_process},
  {NO_DATA, "Shutdown", &shutdown_process},
  {-1, NULL, NULL}
};

static const struct MenuItem AboutMenu[] = 
{
  {DATA_VERSION, "Version", NULL},
  {NO_DATA, "Serial", NULL},
  {DATA_BTADDR, "", NULL},
  {NO_DATA, "Legal", NULL},
  {DATA_LEGAL, "", NULL},
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
  {0,   "About", &menu_process},
  {0, NULL, NULL}
};

static const struct MenuItem TestMenu[] = 
{
  {DATA_BTADDR, "", NULL},
  {0, "Button", &test_button},
  {0, "Motor", &test_motor},
  {0, "Light", &test_light},
  {0, "LCD", &test_lcd},
  {0, "ANT+", &test_ant},
  {0, "MPU6050", &test_mpu6050},
  {0, "Bluetooth", &test_bluetooth},
  {0, "BT DUT", &test_dut},
  {0, "Reboot", &test_reboot},
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
  if (item->icon < 0x80)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
    GrStringDraw(pContext, &item->icon, 1, 10, 14 + (MENU_SPACE - 16) /2 + index * MENU_SPACE, 0);
  }

  GrContextFontSet(pContext, &g_sFontBaby16);
  if (item->icon < 0x80)
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
      case 0xFF:
        return;
      case DATA_DATE:
      {
        uint8_t  month, day;
        uint16_t year;
        rtc_readdate(&year, &month, &day, NULL);
        sprintf(buf, "%s %d %02d", month_shortname[month], day, year - 2000);
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
      case DATA_LEGAL:
      strcpy(buf, "legal.kreyos.com");
      break;
      case DATA_VERSION:
      strcpy(buf, "1.0.0.1");
      break;
      case DATA_BTADDR:
      {
      uint8_t serial[6];
      system_getserial(serial);
      sprintf(buf, "%02X%02X%02X%02X%02X%02X", serial[0], serial[1],serial[2],serial[3],serial[4],serial[5]);
      break;
      }
      default:
      strcpy(buf, "TODO");
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
      
      if (rparam == NULL)
      {
        Items = (system_testing()==1)?TestMenu:MainMenu;
      }
      else if (strcmp(rparam, "Watch Setup") == 0)
      {
        Items = SetupMenu;
      }
      else if (strcmp(rparam, "About") == 0)
      {
        Items = AboutMenu;
      }
      else if (strcmp(rparam, "Test") == 0)
      {
        Items = TestMenu;
      }

      getMenuLength();

      current = currentTop = 0;
      break;
    }
    case EVENT_WINDOW_ACTIVE:
    {
      if (!system_testing())
        etimer_set(&timer, CLOCK_SECOND * 30);
      break;
    }
    case EVENT_WINDOW_DEACTIVE:
    {
      etimer_stop(&timer);
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
            }
            else if (current == 10)
            {
              Items = AboutMenu;              
            }
            getMenuLength();
            current = currentTop = 0;
            window_invalid(NULL);
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
      if (Items == SetupMenu)
      {
        Items = MainMenu;
        currentTop = 5;
        current = 9;
        getMenuLength();
        window_invalid(NULL);
      }
      else if (Items == AboutMenu)
      {
        Items = MainMenu;
        currentTop = 6;
        current = 10;
        getMenuLength();
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

extern uint8_t shutdown_mode;
uint8_t shutdown_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  shutdown_mode = 1;
  
  return 1;
}