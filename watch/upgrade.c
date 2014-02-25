/****************************************************************
*  Description: Implementation for upgrade firmware
*    History:
*      Jun Su          2013/6/21        Created
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include <stdio.h>

// Function prototypes
int CheckUpgrade(void);
void Upgrade(void);

static enum {NOFIRMWARE, W4CONFIRM, CONFIRMED, FAILED} state = W4CONFIRM;

static void onDraw(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrBlack);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrWhite);

	GrContextFontSet(pContext, &g_sFontNova16);
	if (state == W4CONFIRM)
	  GrStringDrawWrap(pContext, "Press Enter key to continue", 0, 30, 150, 30);
	else if (state == CONFIRMED)
	{
	   GrStringDraw(pContext, "Upgrading", -1, 0, 30, 0);
	   GrStringDraw(pContext, "DO NOT UNPLUG", -1, 0, 90, 0);
	}
  else if (state == FAILED)
  {
    GrStringDrawWrap(pContext, "Press Any key to continue", 0, 30, 150, 30);
  }
}

uint8_t upgrade_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  // enable 
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    if (CheckUpgrade())
      state = NOFIRMWARE;
    else
      state = W4CONFIRM;
    break;
  case EVENT_WINDOW_PAINT:
  	onDraw((tContext*)rparam);

  	if (state == CONFIRMED)
  		window_timer(CLOCK_SECOND);
  	break;
  case EVENT_KEY_PRESSED:
    switch(state)
    {
  	 case W4CONFIRM:
        if (lparam == KEY_ENTER)
        {
          state = CONFIRMED;
    	    window_invalid(NULL);
        }
       break;
  	 case FAILED:
     case NOFIRMWARE:
        window_close();
        break;
    }
    break;
  case PROCESS_EVENT_TIMER:
  	if (state == CONFIRMED)
    {
      Upgrade();
  	  state = FAILED;
      window_invalid(NULL);
    }
    break;
  default:
  	return 0;
  }
  return 1;
}