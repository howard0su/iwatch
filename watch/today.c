#include "contiki.h"

#include "window.h"
#include "pedometer/pedometer.h"
#include <stdio.h>
#include "grlib/grlib.h"
#include "Template_Driver.h"

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
      GrStringDraw(pContext, &icon, 1, 8, 30 + n * 35, 0);
    }

  // draw text
  GrContextFontSet(pContext, &g_sFontNova13);
  GrStringDrawWrap(pContext, text, 30, 30 + n * 35, LCD_X_SIZE / 2, 16);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_X_SIZE - width - 8, 30 + n * 35, 0);
}

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova38);

  sprintf(buf, "%d", ped_get_steps());
  drawItem(pContext, 0, 'l', "Steps Taken", buf);

  sprintf(buf, "%d", ped_get_time());
  drawItem(pContext, 1, 0, "Walk Time", buf);

  sprintf(buf, "%d", ped_get_calorie());
  drawItem(pContext, 2, 0, "Caloris", buf);

  sprintf(buf, "%d m", ped_get_distance());
  drawItem(pContext, 3, 0, "Distance", buf);

  // draw progress
}

uint8_t today_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
  case PROCESS_EVENT_TIMER:
    window_timer(CLOCK_SECOND * 5);
    window_invalid(NULL);
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
