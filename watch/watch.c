/****************************************************************
*  Description: Implementation for System process
*    History:
*      Jun Su          2013/1/2        Created
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
#include "backlight.h"

#include <stdlib.h>
#include <stdio.h> /* For printf() */

extern tImage g_logoImage;
/*
* This process is the startup process.
* It first shows the logo
* Like the whole dialog intrufstture.
*/
uint8_t watch_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    {
      backlight_on(255);
      window_timer(CLOCK_SECOND);
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      tContext *pContext = (tContext*)rparam;
      GrContextForegroundSet(pContext, ClrWhite);
      GrImageDraw(pContext, &g_logoImage, 0, 60);
      break;
    }
  case PROCESS_EVENT_TIMER:
    {
      backlight_on(0);
      window_open(&menu_process, NULL);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    {
      break;
    }
  default:
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
