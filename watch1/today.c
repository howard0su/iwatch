#include "contiki.h"

#include "window.h"
#include "pedometer/pedometer.h"
#include <stdio.h>
#include "icons.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include "memory.h"

enum {
  WALK = 0,
  SPORT = 1
};

#define state d.today.state

#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
#define LINEMARGIN 35
#else
#define LINEMARGIN 30
#endif
static void drawItem(tContext *pContext, uint8_t n, char icon, const char* text, const char* value)
{
  if (icon)
    {
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcons32);
      GrStringDraw(pContext, &icon, 1, 12, 21 + n * LINEMARGIN, 0);
#else    	
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
      GrStringDraw(pContext, &icon, 1, 3, 12 + n * LINEMARGIN, 0);
#endif      
    }
    // draw text
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
  GrContextFontSet(pContext, &g_sFontBaby16);
  GrStringDraw(pContext, text, -1, 57, 24 + n * LINEMARGIN, 0);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, 57, 39 + n * LINEMARGIN, 0);
#else  
  GrContextFontSet(pContext, &g_sFontGothic24b);
  GrStringDraw(pContext, text, -1, 20, 10 + n * LINEMARGIN, 0);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_WIDTH - width - 4, 10 + n * LINEMARGIN, 0);
#endif  
}

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
	GrRectFill(pContext, &client_clip);
#else  
  GrRectFill(pContext, &fullscreen_clip);
#endif
  GrContextForegroundSet(pContext, ClrWhite);

  if (state == WALK)
  {
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
    sprintf(buf, "%d", ped_get_steps());
    drawItem(pContext, 0, ICON_STEPS, "Step Taken", buf);
    
    sprintf(buf, "%02d:%02d", ped_get_time() / 60, ped_get_time() % 60);
    drawItem(pContext, 1, ICON_TIME, "Walk Time", buf);

    uint16_t cals = ped_get_calorie() / 100 / 1000;
    sprintf(buf, "%d", cals);
    drawItem(pContext, 2, ICON_CALORIES, "Calories", buf);
    
    uint16_t dist = ped_get_distance() / 100;
    sprintf(buf, "%dm", dist);
    drawItem(pContext, 3, ICON_DISTANCE, "Distance", buf);    
    
#else  	
    sprintf(buf, "%d", ped_get_steps());
    drawItem(pContext, 0, ICON_STEPS, "Steps", buf);

    uint16_t cals = ped_get_calorie() / 100 / 1000;
    sprintf(buf, "%d", cals);
    drawItem(pContext, 1, ICON_CALORIES, "Calories", buf);

    uint16_t dist = ped_get_distance() / 100;
    sprintf(buf, "%dm", dist);
    drawItem(pContext, 2, ICON_DISTANCE, "Distance", buf);

    sprintf(buf, "%02d:%02d", ped_get_time() / 60, ped_get_time() % 60);
    drawItem(pContext, 3, ICON_TIME, "Active", buf);
#endif
    // draw progress
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)
#else
    uint16_t goal = window_readconfig()->goal_steps;
    uint32_t steps = ped_get_steps();

    if (steps < goal)
      window_progress(pContext, 5 + 4 * LINEMARGIN, steps * 100 / goal);
    else
      window_progress(pContext, 5 + 4 * LINEMARGIN, 100);
    sprintf(buf, "%d%% of %d", (uint16_t)(steps * 100 / goal), goal);
    GrContextForegroundSet(pContext, ClrWhite);
    GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH/2, 148, 0);
#endif    
  }
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
    return 0x80;
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
