#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "system.h"
#include "Template_Driver.h"

uint8_t reset_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
    case EVENT_WINDOW_PAINT:
    {
      tContext *pContext = (tContext*)rparam;
      GrContextForegroundSet(pContext, ClrBlack);
      GrRectFill(pContext, &client_clip);

      GrContextForegroundSet(pContext, ClrWhite);
      GrContextFontSet(pContext, &g_sFontNova16);
	  GrStringDrawCentered(pContext, "Reset Watch", -1, LCD_X_SIZE/2, 40, 0);
	  
	  GrContextFontSet(pContext, &g_sFontNova16b);
	  GrStringDrawCentered(pContext, "Confirm?", -1, LCD_X_SIZE/2, 100, 0);

	  window_button(pContext, KEY_DOWN, "Reset");
      return 1;
    }

    case EVENT_KEY_PRESSED:
    if (lparam == KEY_DOWN)
    {
    	system_resetfactory();
    }
    break;
  }

  return 0;
}
