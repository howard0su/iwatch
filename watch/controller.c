/****************************************************************
*  Description: Implementation for remote control
*    History:
*      Jun Su          2013/3/3        Created
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
#include "avctp.h"
#include "avrcp.h"

/*
 * This implement the menu
 */
PROCESS(control_process, "Controller Window");

PROCESS_THREAD(control_process, ev, data)
{
  PROCESS_BEGIN();

  while(1)
  {
    if (ev == EVENT_WINDOW_CREATED)
    {
    }
    else if (ev == EVENT_KEY_PRESSED)
    {
      if ((uint8_t)data == KEY_UP)
      {
        avctp_send_passthrough(PLAY_OP);
      }
      else if ((uint8_t)data == KEY_DOWN)
      {
        avctp_send_passthrough(PAUSE_OP);
      }
      else if ((uint8_t)data == KEY_ENTER)
      {
        avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);

      }
      else if ((uint8_t)data == KEY_EXIT)
      {
        avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
      }
    }
    else
    {
      window_defproc(ev, data);
    }

    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}