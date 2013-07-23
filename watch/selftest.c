#include "contiki.h"
#include "window.h"
#include "gesture.h"
#include "battery.h"
#include "hfp.h"
#include <stdio.h>

uint8_t selftest_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
    {
    case EVENT_WINDOW_CREATED:
      {
		window_timer(CLOCK_SECOND * 5);
		break;
      }
    case EVENT_WINDOW_PAINT:
      {
	tContext *pContext = (tContext*)rparam;
	GrContextForegroundSet(pContext, ClrBlack);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrWhite);

	GrContextFontSet(pContext, &g_sFontNova16);
	const char* msg;
	// draw the state
	switch(battery_state())
	  {
	  case BATTERY_DISCHARGING:
	    msg = "battery is discharging.";
	    break;
	  case BATTERY_CHARGING:
	    msg = "battery is charging.";
	    break;
	  case BATTERY_FULL:
	    msg = "battery is full. charged stopped.";
	    break;
	  }
	GrStringDraw(pContext, msg, -1, 10, 20, 0);
	char buf[50];
	uint8_t level = battery_level();
	sprintf(buf, "battery level is %d\n", level);
	GrStringDraw(pContext, buf, -1, 10, 40, 0);

#if ENERGEST_CONF_ON
	sprintf(buf, "cpu %lu lpm %lu irq %lu serial %lu\n",
	 energest_total_time[ENERGEST_TYPE_CPU].current,
	 energest_total_time[ENERGEST_TYPE_LPM].current,
	 energest_total_time[ENERGEST_TYPE_IRQ].current,
	 energest_total_time[ENERGEST_TYPE_SERIAL].current);

  printf("cpu %lu lpm %lu irq %lu serial %lu\n",
	 energest_total_time[ENERGEST_TYPE_CPU].current,
	 energest_total_time[ENERGEST_TYPE_LPM].current,
	 energest_total_time[ENERGEST_TYPE_IRQ].current,
	 energest_total_time[ENERGEST_TYPE_SERIAL].current);

	GrStringDrawWrap(pContext, buf, 0, 55, 100, 15);
#endif
	window_timer(CLOCK_SECOND * 5);
	break;
      }
    case PROCESS_EVENT_TIMER:
      window_invalid(NULL);
      break;
    case EVENT_KEY_PRESSED:
    	if (lparam == KEY_ENTER)
      		hfp_enable_voicerecog();
      	else if (lparam == KEY_UP)
      	{
      		printf("\nStart Recoding...\n");
      		gesture_init(1); // recording
      	}
      	else if (lparam == KEY_DOWN)
      	{
      		printf("\nStart Recongize...\n");
      		gesture_init(0); // recording	
      	}
      break;
    case EVENT_WINDOW_CLOSING:
      window_timer(0);
      break;
    default:
      return 0;
    }

  return 1;
}
