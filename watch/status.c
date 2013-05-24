#include "contiki.h"
#include "sys/ctimer.h"

#include "grlib/grlib.h"
#include "window.h"
#include "Template_Driver.h"

extern const tRectangle status_clip;

#define ICON_RUN        'h'
#define ICON_ALARM      'j'
#define ICON_BT         'k'

#define ICON_BATTERY_FULL 'm'
#define ICON_BATTERY_MORE 'n'
#define ICON_BATTERY_LESS 'o'
#define ICON_BATTERY_EMPTY 'p'
#define ICON_BATTERY_CHARGE_LESS 'q'
#define ICON_BATTERY_CHARGE_MORE 'r'

#define BATTERY_X 125
#define BT_X 107
#define ALARM_X 89

static void OnDraw(tContext* pContext)
{
  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &status_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDrawH(pContext, 0, LCD_X_SIZE, 16);

  GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
  char icon;
  icon = ICON_BT;
  GrStringDraw(pContext, &icon, 1, BT_X, 0, 0);
  icon = ICON_ALARM;
  GrStringDraw(pContext, &icon, 1, ALARM_X, 0, 0);
  icon = ICON_BATTERY_LESS;
  GrStringDraw(pContext, &icon, 1, BATTERY_X, 0, 0);
}

uint8_t status_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    status_invalid();
    break;
  case EVENT_WINDOW_PAINT:
    OnDraw((tContext*)rparam);
    break;
  case EVENT_BT_STATUS:
    break;
  case EVENT_ANT_STATUS:
    break;
  default:
    return 0;
  }

  return 1;
}
