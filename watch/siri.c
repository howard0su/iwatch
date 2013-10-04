/*
* This file implment the notification for the incoming phone
*/

#include "contiki.h"
#include "window.h"
#include "backlight.h"
#include "hfp.h"
#include "bluetooth.h"
#include <string.h>
#include <stdio.h>

static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);
  GrContextForegroundSet(pContext, ClrWhite);
  
  GrContextFontSet(pContext, &g_sFontBaby16);
  GrStringDraw(pContext, "Voice Command", -1, 32, 16, 0);
  window_button(pContext, KEY_EXIT, "Hang up");
}

uint8_t siri_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    if (!hfp_connected())
      window_close();
    
    if (rparam)
      hfp_enable_voicerecog(1);
    break;
  case EVENT_BT_BVRA:
    if (lparam == 0)
    {
      window_close();
    }
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
    
  case EVENT_EXIT_PRESSED:
    hfp_enable_voicerecog(0);
    break;
    
  default:
    return 0;
  }
  
  return 1;
}
