#include "contiki.h"
#include "window.h"

static uint8_t progress;
extern int CheckUpgrade();
static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);
  GrContextForegroundSet(pContext, ClrWhite);
  
  GrContextFontSet(pContext, &g_sFontBaby16);
  GrStringDrawCentered(pContext, "Firmware Upgrade", -1, 72, 60, 0);
  if (progress == 100)
  	GrStringDrawCentered(pContext, "Done", -1, 72, 96, 0);
  else
  	GrStringDrawCentered(pContext, "Don't do any operation", -1, 72, 96, 0);

  if (progress == 100)
  	window_button(pContext, KEY_ENTER, "Reboot");
  else
	window_progress(pContext, 110, progress);
}

extern uint8_t upgrade_process(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_FIRMWARE_UPGRADE:
			if (rparam == (void*)-1)
			{
				progress = 100;
			}
			else
			{
				progress = (long)rparam * 100/(230UL*1024);
			}
			window_invalid(NULL);
			break;
		case EVENT_WINDOW_PAINT:
			onDraw((tContext*)rparam);
		    break;
		case EVENT_EXIT_PRESSED:
			if (progress != 100)
				return 1;
			break;
		case EVENT_KEY_PRESSED:
		 	if (lparam == KEY_ENTER)
		 	{
		 		if (progress == 100)
		 		{
		 			if (!CheckUpgrade())
		 				system_reset();
		 			else
		 				window_close();
		 		}
		 	}
		 	break;
	}
}