#include "contiki.h"
#include "window.h"

#include "battery.h"
#include "hfp.h"

uint8_t selftest_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
    {
    case EVENT_WINDOW_CREATED:
      {
	window_timer(CLOCK_SECOND);
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
	break;
      }
    case PROCESS_EVENT_TIMER:
      window_invalid(NULL);
      break;
    case EVENT_KEY_PRESSED:
      hfp_enable_voicerecog();
      break;
    case EVENT_WINDOW_CLOSING:
      window_timer(0);
      break;
    default:
      return 0;
    }

  return 1;
}
