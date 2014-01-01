#include "contiki.h"

#include "window.h"
#include "pedometer/pedometer.h"
#include <stdio.h>
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "memory.h"

static enum {
  WALK = 0,
  SPORT = 1
};

#define state d.today.state


#define LINEMARGIN 25
static void drawItem(tContext *pContext, uint8_t n, char icon, const char* text, const char* value)
{
  if (icon)
    {
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
      GrStringDraw(pContext, &icon, 1, 8, 30 + n * LINEMARGIN, 0);
    }

  // draw text
  GrContextFontSet(pContext, &g_sFontNova13);
  GrStringDraw(pContext, text, -1, 30, 30 + n * LINEMARGIN, 0);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_X_SIZE - width - 8, 30 + n * LINEMARGIN, 0);
}

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova38);

  if (state == WALK)
  {
    sprintf(buf, "%d", ped_get_steps());
    drawItem(pContext, 0, 'y', "Steps Taken", buf);

    sprintf(buf, "%02d:%02d", ped_get_time() / 60, ped_get_time() % 60);
    drawItem(pContext, 1, 'z', "Walk Time", buf);

    sprintf(buf, "%dkcal", ped_get_calorie());
    drawItem(pContext, 2, 'z'+1, "Calorie", buf);

    sprintf(buf, "%dm", ped_get_distance());
    drawItem(pContext, 3, 'z'+2, "Distance", buf);

    // draw progress

    int goal = window_readconfig()->goal_steps;
    int steps = ped_get_steps();

    window_progress(pContext, 28 + 4 * LINEMARGIN, steps * 100 / goal);
    sprintf(buf, "%d%% of goal of %d", steps < goal ? steps * 100 / goal:100, goal);
    GrContextForegroundSet(pContext, ClrWhite);
    GrStringDrawCentered(pContext, buf, -1, LCD_X_SIZE/2, 144, 0);
  }
  #if 0
  else
  {
    sprintf(buf, "%d", ped_get_steps());
    drawItem(pContext, 0, 'y', "Steps Taken", buf);

    sprintf(buf, "%d", ped_get_time());
    drawItem(pContext, 1, 'z', "Walk Time", buf);

    sprintf(buf, "%d", ped_get_calorie());
    drawItem(pContext, 2, 'z'+1, "Caloris", buf);

    sprintf(buf, "%d m", ped_get_distance());
    drawItem(pContext, 3, 'z'+2, "Distance", buf);    
  }
  GrContextForegroundSet(pContext, ClrWhite);
  for(int i = 0; i < 6; i++)
  {
    GrLineDrawH(pContext, 130 - i, 130 + i,  25 + i);

    GrLineDrawH(pContext, 130 - i, 130 + i,  160 - i);
  }
#endif
}

uint8_t today_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    state = WALK;
    // fallthrough
  case PROCESS_EVENT_TIMER:
    window_timer(CLOCK_SECOND * 5);
    window_invalid(NULL);
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
#if 0
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_DOWN || lparam == KEY_UP)
    {
      state = 1 - state;
    }
    window_invalid(NULL);
    break;
#endif
  default:
    return 0;
  }

  return 1;
}
