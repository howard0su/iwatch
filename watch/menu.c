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
#include "memory.h"
#include "sportsdata.h"

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
#define DATA_LIGHT 0xF8
#define DATA_VOL  0xF9
#define NO_DATA   0xFF

static const struct MenuItem SetupMenu[] =
{
  {DATA_DATE, "Date", &configdate_process},
  {DATA_TIME, "Time", &configtime_process},
  {DATA_LIGHT, "Back Light", &configlight_process},
  {DATA_VOL, "Volume", &configvol_process},
  {DATA_BT, "Bluetooth", &btconfig_process},
  {NO_DATA, "Upgrade Firmware", &upgrade_process},
//  {NO_DATA, "Shutdown", &shutdown_process},
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
  {'a', "History", &menu_process},
  {'l', "Setup", &menu_process},
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
  {NO_DATA, "Self-test", &selftest_process},
  {0, "Reboot", &test_reboot},
  {0, NULL, NULL}
};

extern struct process filesys_process;
extern uint8_t recordoperation_process(uint8_t ev, uint16_t lparam, void* rparam);

#define HistoryActivity d.menu.HistoryActivity
#define history_names   d.menu.displaynames
#define file_names      d.menu.filenames

#define SET_MENU_END(id, menu) menu[id].name = NULL, menu[id].handler = NULL, menu[id].icon = 0

static uint8_t s_record_pos = 0;
static uint8_t loadHistoryRecord(char* filename)
{
    uint8_t pos = s_record_pos;
    if (pos >= count_elem(HistoryActivity) - 1)
    {
        printf("max history entris reach:%d\n", count_elem(HistoryActivity) - 1);
        return 0;
    }

    record_desc_t record;
    if(get_record_desc(filename, &record) != 0 && !record.is_continue)
    {
        printf("load %s\n", filename);
        strcpy(file_names[pos], filename);
        if (record.mode != DATA_MODE_NORMAL || record.sec != 0)
        {
            sprintf(history_names[pos], "%02d/%02d/%02d %02d:%02d",
                    record.month, record.day, record.year, record.hour, record.min);
        }
        else
        {
            sprintf(history_names[pos], "%02d/%02d/%02d",
                    record.month, record.day, record.year);
        }
        HistoryActivity[pos].handler = &menu_process;
        HistoryActivity[pos].icon    = record.mode == DATA_MODE_RUNNING ? 'h' : 'a';
        HistoryActivity[pos].name    = history_names[pos];

        s_record_pos++;
        SET_MENU_END(s_record_pos, HistoryActivity);
    }
    return 1;
}

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

  tRectangle rect = {8, 17 + index * MENU_SPACE, 134, 9 + (index + 1) * MENU_SPACE};
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
        sprintf(buf, "%s %d, %04d", month_shortname[month - 1], day, year);
        break;
      }
      case DATA_TIME:
      {
        uint8_t hour, minute;
        char buf0[2];
        uint8_t ampm = 0;
        rtc_readtime(&hour, &minute, NULL);
          // draw time
        if (hour > 12)
        {
          ampm = 1; // pm
          hour -= 12;
        }  
        if (ampm) buf0[0] = 'P';
          else buf0[0] = 'A';
        buf0[1] = 'M';

        sprintf(buf, "%02d:%02d %c%c", hour, minute, buf0[0], buf0[1]);
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
      case DATA_LIGHT:
      sprintf(buf, "%d", window_readconfig()->light_level);
      break;
      case DATA_VOL:
      sprintf(buf, "%d", window_readconfig()->volume_level);
      break;
      case DATA_BTADDR:
      {
      sprintf(buf, "%s", bluetooth_address());
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

  if (NUM_MENU_A_PAGE < menuLength)
  {
    // draw progress bar
    #define STEPS 128
    int length = NUM_MENU_A_PAGE * STEPS / menuLength;
    int start = currentTop * STEPS / menuLength;

    tRectangle rect = {136, 20, 143, 20 + STEPS + 10};
    GrContextForegroundSet(pContext, ClrWhite);
    GrRectFillRound(pContext, &rect, 3);
    GrContextForegroundSet(pContext, ClrBlack);

    rect.sXMin += 2;
    rect.sXMax -= 2;

    rect.sYMin += 4 + start;
    rect.sYMax = rect.sYMin + length;
    GrRectFill(pContext, &rect);
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
      else if (strcmp(rparam, "History") == 0)
      {
        Items = HistoryActivity;
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
        if (Items == HistoryActivity)
        {
            window_open(&recordoperation_process, file_names[current]);
        }
        else if (Items[current].handler)
        {
          if (Items[current].handler == &menu_process)
          {
            if (current == 9)
            {
              s_record_pos = 0;
              SET_MENU_END(0, HistoryActivity);
              Items = HistoryActivity;
              process_post(&filesys_process, PROCESS_EVENT_READ_DIR, NULL);
            }
            else if (current == 10)
            {
              Items = SetupMenu;
            }
            else if (current == 11)
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
        currentTop = 6;
        current = 10;
        getMenuLength();
        window_invalid(NULL);
      }
      else if (Items == AboutMenu)
      {
        Items = MainMenu;
        currentTop = 7;
        current = 11;
        getMenuLength();
        window_invalid(NULL);
      }
      else if (Items == HistoryActivity)
      {
        Items = MainMenu;
        currentTop = 5;
        current = 9;
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

    case EVENT_FILESYS_LIST_FILE:
      if ((int)rparam != 0 && (int)rparam != -1)
      {
        loadHistoryRecord((char*)rparam);
        process_post(&filesys_process, PROCESS_EVENT_READ_DIR_PROC, NULL);
      }
      else
        window_invalid(NULL);
      break;

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
