#include "contiki.h"
#include "sys/ctimer.h"
#include "mpu.h"
#include "grlib/grlib.h"
#include "window.h"
#include "Template_Driver.h"
#include "btstack/bluetooth.h"
#include "battery.h"
#include "rtc.h"
#include "pedometer/pedometer.h"

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
    GrStringDraw(pContext, &icon, 1, 48, 0, 0);
    uint16_t steps;
    // todo: fetch goal
    int part = window_readconfig()->goal_steps / 5;
    steps = ped_get_steps();
    for(int i = 0; i < 5; i++)
    {
      tRectangle rect = {64 + i * 6, 6, 68 + i * 6, 9};
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
    uint8_t ampm = 0;
    rtc_readtime(&hour, &minute, NULL);

    if (hour > 12)
    {
      hour -= 12;
      ampm = 1;
    }
    sprintf(buf, "%02d:%02d%s", hour, minute, ampm?"PM":"AM");
    GrContextFontSet(pContext, &g_sFontNova12b);
    int width = GrStringWidthGet(pContext, buf, -1);
    GrStringDraw(pContext, buf, -1, (LCD_X_SIZE - width)/2, 0, 0);
  }
}

/*
 battery is between 3.7 to 4.2
  4.2 => 2.1 / 2.5 * 255 = 214
  3.7 => 1.86 / 2.5 * 255 = 189
*/
static void check_battery()
{
  // update battery status
  uint8_t level = battery_level();
  uint8_t state = battery_state();

  status &= ~BATTERY_STATUS;
  if (state == BATTERY_DISCHARGING)
  {
    switch(level)
    {
      case 0: case 1: case 2:
      status |= BATTERY_EMPTY;
      break;
      case 3: case 4: case 5: case 6:
      status |= BATTERY_LESS;
      break;
      case 7: case 8: case 9: case 10: case 11: case 12: case 13:
      status |= BATTERY_MORE;
      break;
      default:
      status |= BATTERY_FULL;
  }
  }
  else if (state == BATTERY_CHARGING)
  {
    if (level >= 15)
    {
      status |= BATTERY_FULL;
    }
    else if ((status & BATTERY_STATUS) == BATTERY_MORE_CHARGE)
    {
      status |= BATTERY_LESS_CHARGE;
    }
    else
    {
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
    {
      uint8_t hour, minute, second;
      rtc_readtime(&hour, &minute, &second);
      if (hour == 0 && minute == 0 && second <= 19)
      {
        ped_reset();
      }
    check_battery();
      //write_walkstatus();
    status ^= MID_STATUS;
    break;
    }
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
