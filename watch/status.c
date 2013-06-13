#include "contiki.h"
#include "sys/ctimer.h"
#include "mpu.h"
#include "grlib/grlib.h"
#include "window.h"
#include "Template_Driver.h"
#include "btstack/bluetooth.h"
#include "battery.h"
#include "rtc.h"

#include <stdio.h>

extern const tRectangle status_clip;

#define ICON_RUN        'h'
#define ICON_ALARM      'i'
#define ICON_BT         'j'

#define ICON_BATTERY_FULL 'm'
#define ICON_BATTERY_MORE 'n'
#define ICON_BATTERY_LESS 'o'
#define ICON_BATTERY_EMPTY 'p'
#define ICON_BATTERY_CHARGE_LESS 'q'
#define ICON_BATTERY_CHARGE_MORE 'r'

#define BATTERY_X 125
#define BT_X 107
#define ALARM_X 89

#define BATTERY_STATUS 0x07 // bit0 bit1 bit2 for battery
#define BLUETOOTH_STATUS 0x08
#define ANTPLUS_STATUS 0x10
#define ALARM_STATUS  0x20
#define MID_STATUS    0x40

#define BATTERY_EMPTY 1
#define BATTERY_LESS  2
#define BATTERY_MORE  3
#define BATTERY_FULL  4
#define BATTERY_LESS_CHARGE 5
#define BATTERY_MORE_CHARGE 6

static uint16_t status;

static void OnDraw(tContext* pContext)
{
  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &status_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDrawH(pContext, 0, LCD_X_SIZE, 16);

  GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
  char icon;

  if (status & BLUETOOTH_STATUS)
  {
    icon = ICON_BT;
    GrStringDraw(pContext, &icon, 1, BT_X, 0, 0);
  }

  if (status & ALARM_STATUS)
  {
    icon = ICON_ALARM;
    GrStringDraw(pContext, &icon, 1, ALARM_X, 0, 0);
  }

  switch(status & BATTERY_STATUS)
  {
  case BATTERY_EMPTY:
    icon = ICON_BATTERY_EMPTY;
    break;
  case BATTERY_LESS:
    icon = ICON_BATTERY_LESS;
    break;
  case BATTERY_MORE:
    icon = ICON_BATTERY_MORE;
    break;
  case BATTERY_FULL:
    icon = ICON_BATTERY_FULL;
    break;
  case BATTERY_LESS_CHARGE:
    icon = ICON_BATTERY_CHARGE_LESS;
    break;
  case BATTERY_MORE_CHARGE:
    icon = ICON_BATTERY_CHARGE_MORE;
    break;
  default:
    icon = 0;
  }

  if (icon != 0)
    GrStringDraw(pContext, &icon, 1, BATTERY_X, 0, 0);
  if (status & MID_STATUS)
  {
    char icon = ICON_RUN;
    // draw activity
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
    GrStringDraw(pContext, &icon, 1, 54, 0, 0);
    unsigned long steps;
    // todo: fetch goal
    int part = 12000 / 5;
    steps = 100;
    for(int i = 1; i < 6; i++)
    {
      tRectangle rect = {67 + i * 5, 6, 70 + i * 5, 9};
      if (i * part <= steps)
      {
        GrRectFill(pContext, &rect);
      }
      else
      {
        GrRectDraw(pContext, &rect);
      }
    }
  }
  else
  {
    uint8_t hour, minute;
    char buf[20];
    rtc_readtime(&hour, &minute, NULL);
    sprintf(buf, "%02d:%02d", hour, minute);
    GrContextFontSet(pContext, &g_sFontNova12b);
    int width = GrStringWidthGet(pContext, buf, -1);
    GrStringDraw(pContext, buf, -1, (LCD_X_SIZE - width)/2, 0, 0);
  }
}

static void check_battery()
{
  // update battery status
  uint8_t level = battery_level();
  uint8_t state = battery_state();

  if (state == BATTERY_DISCHARGING)
  {
    status &= ~BATTERY_STATUS;
    // not charging
    if (level < 50)
      status |= BATTERY_EMPTY;
    else if (level < 120)
      status |= BATTERY_LESS;
    else if (level < 200)
      status |= BATTERY_MORE;
    else
      status |= BATTERY_FULL;
  }
  else if (state == BATTERY_CHARGING)
  {
    if ((status & BATTERY_STATUS) == BATTERY_MORE_CHARGE)
    {
      status &= ~BATTERY_STATUS;
      status |= BATTERY_LESS_CHARGE;
    }
    else
    {
      status &= ~BATTERY_STATUS;
      status |= BATTERY_MORE_CHARGE;
    }
  }
}

uint8_t status_process(uint8_t event, uint16_t lparam, void* rparam)
{
  uint8_t old_status = status;
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    status = 0;
    check_battery();
    status_invalid();
    break;
  case EVENT_WINDOW_PAINT:
    OnDraw((tContext*)rparam);
    break;
  case PROCESS_EVENT_TIMER:
    check_battery();
    status ^= MID_STATUS;
    break;
  case EVENT_BT_STATUS:
    if (lparam == BT_INITIALIZED)
      status |= BLUETOOTH_STATUS;
    else if (lparam == BT_SHUTDOWN)
      status &= ~BLUETOOTH_STATUS;
    break;
  case EVENT_ANT_STATUS:
    break;
  default:
    return 0;
  }

  if (status != old_status)
    status_invalid();
  return 1;
}
