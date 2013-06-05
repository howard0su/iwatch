#include "contiki.h"

#include "window.h"
#include "mpl/inv_mpu_dmp_motion_driver.h"
#include <stdio.h>
#include "grlib/grlib.h"
#include "Template_Driver.h"

static unsigned long steps, time, cal;
static enum {
  STEPS,
  TIME,
  CALORIES,
}state = STEPS;

static void drawItem(tContext *pContext, uint8_t n, char icon, const char* text, const char* value)
{
  if (icon)
    {
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
      GrStringDraw(pContext, &icon, 1, 8, 30 + n * 40, 0);
    }

  // draw text
  GrContextFontSet(pContext, &g_sFontNova13);
  GrStringDrawWrap(pContext, text, 30, 30 + n * 45, LCD_X_SIZE/2 - 30, 16);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_X_SIZE - width - 8, 30 + n * 45, 0);
}

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova38);

  sprintf(buf, "%d", steps);
  drawItem(pContext, 0, 'l', "Steps Taken", buf);

  sprintf(buf, "%d", time);
  drawItem(pContext, 1, 0, "Walk Time", buf);

  sprintf(buf, "%d", cal);
  drawItem(pContext, 2, 0, "Caloris", buf);

  sprintf(buf, "%d mi", 12);
  drawItem(pContext, 3, 0, "Distinct", buf);

  // draw progress
}

uint8_t today_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    dmp_get_pedometer_step_count(&steps);
    dmp_get_pedometer_walk_time(&time);
    cal = 130;
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_DOWN)
    {
      state++;
      if (state > CALORIES)
        state = STEPS;
    }
    else if (lparam == KEY_UP)
    {
      if (state != 0)
        state--;
    }
    window_invalid(NULL);
    break;
  default:
    return 0;
  }

  return 1;
}
