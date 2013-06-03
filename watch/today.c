#include "contiki.h"

#include "window.h"
#include "mpl/inv_mpu_dmp_motion_driver.h"
#include <stdio.h>

static unsigned long steps, time;
static enum {
  STEPS,
  TIME,
  CALORIES,
}state = STEPS;

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova38);

  switch(state)
  {
    case STEPS:
      sprintf(buf, "%ld", steps);
      window_progress(pContext, 17, steps / 100);
      GrContextForegroundSet(pContext, ClrWhite);      
      GrContextFontSet(pContext, &g_sFontNova38);
      GrStringDraw(pContext, buf, -1, 12, 65, 0);
      GrContextFontSet(pContext, &g_sFontNova16);
      GrStringDraw(pContext, "Steps taken", -1, 10, 100, 0);
      break;
    case TIME:
      sprintf(buf, "%ld", time);
      window_progress(pContext, 17, time / 100);
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextFontSet(pContext, &g_sFontNova38);
      GrStringDraw(pContext, buf, -1, 12, 65, 0);
      GrContextFontSet(pContext, &g_sFontNova16);
      GrStringDraw(pContext, "Minutes Walk taken", -1, 10, 100, 0);
      break;
    case CALORIES:
      sprintf(buf, "%ld", time);
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextFontSet(pContext, &g_sFontNova38);
      GrStringDraw(pContext, buf, -1, 12, 65, 0);
      GrContextFontSet(pContext, &g_sFontNova16);
      GrStringDraw(pContext, "Calories Burned", -1, 10, 100, 0);
      break;
  }

  for (uint8_t i = 0; i < 8; i++)
  {
    tRectangle rect;
    rect.sXMin = 17 + i * 15;
    rect.sXMax = 27 + i * 15;
    rect.sYMin = 146;
    rect.sYMax = 156;
    if (i == state)
      GrRectFill(pContext, &rect);
    else
      GrRectDraw(pContext, &rect);
  }
}

uint8_t today_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    dmp_get_pedometer_step_count(&steps);
    dmp_get_pedometer_walk_time(&time);
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