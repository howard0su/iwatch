#include "contiki.h"
#include "sys/ctimer.h"
#include "grlib/grlib.h"
#include "window.h"
#include "Template_Driver.h"
#include "btstack/bluetooth.h"
#include "battery.h"
#include "rtc.h"
#include "pedometer/pedometer.h"
#include "sportsdata.h"
#include "memory.h"

#include <stdio.h>

extern const tRectangle status_clip;
extern void hfp_battery(int level);

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

void adjustAMPM(uint8_t hour, uint8_t *outhour, uint8_t *ampm);

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
    for(int i = 1; i <= 5; i++)
    {
      tRectangle rect = {64 + i * 6, 6, 68 + i * 6, 9};
      if (i * part / 2 <= steps)
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
    uint8_t ampm;
    rtc_readtime(&hour, &minute, NULL);

    adjustAMPM(hour, &hour, &ampm);

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
  uint8_t report = 0;
  // update battery status
  uint8_t level = battery_level();
  uint8_t state = battery_state();

  status &= ~BATTERY_STATUS;
  if (state == BATTERY_DISCHARGING)
  {
    switch(level)
    {
      case 0: case 1:
      status |= BATTERY_EMPTY;
      break;
      case 2: case 3: case 4:
      status |= BATTERY_LESS;
      break;
      case 5: case 6: case 7:
      status |= BATTERY_MORE;
      break;
      default:
      status |= BATTERY_FULL;
  }
  }
  else if (state == BATTERY_CHARGING)
  {
    if (level >= 9)
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

  hfp_battery(level);
}


static uint8_t s_old_data_hour = 0;
static uint8_t s_old_data_min  = 0;
static uint8_t s_has_old_data  = 0;

static uint32_t s_daily_data[3] = {0};
static uint8_t s_cur_min = 0;

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

      if (hour == 0 && minute == 0 && second <= 30)
      {
        ped_reset();

        memset(&s_daily_data, 0, sizeof(s_daily_data));

        uint16_t year;
        uint8_t month, day, weekday;
        rtc_readdate(&year, &month, &day, &weekday);
        create_data_file(year - 2000, month, day);
      }

      if (s_cur_min != minute &&
        (get_mode() == DATA_MODE_NORMAL || (get_mode() & DATA_MODE_PAUSED) != 0))
      {
        s_cur_min = minute;

        uint32_t data[3] = {0};
        data[0] = ped_get_steps()    - s_daily_data[0];
        data[1] = ped_get_calorie()  - s_daily_data[1];
        data[2] = ped_get_distance() - s_daily_data[2];

        if (data[0] != 0 || data[1] != 0 || data[2] != 0)
        {
          uint8_t meta[] = {DATA_COL_STEP, DATA_COL_CALS, DATA_COL_DIST};
          if (s_has_old_data)
          {
            //write last turn data
            uint32_t old_data[3] = {0};
            write_data_line(get_mode(), s_old_data_hour, s_old_data_min, meta, old_data, sizeof(old_data) / sizeof(old_data[0]));
            s_has_old_data  = 0;
          }

          //write this turn data
          write_data_line(get_mode(), hour, minute, meta, data, sizeof(data) / sizeof(data[0]));
  
          s_daily_data[0] += data[0];
          s_daily_data[1] += data[1];
          s_daily_data[2] += data[2];
        }
        else
        {
          s_old_data_hour = hour;
          s_old_data_min  = minute;
          s_has_old_data  = 1;
        }
      }

      //if (minute % 5 == 0)
      check_battery();
      
      status ^= MID_STATUS;
      break;
    }
  case EVENT_BT_STATUS:
    if (lparam == BT_CONNECTED)
      status |= BLUETOOTH_STATUS;
    else if (lparam == BT_DISCONNECTED)
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
