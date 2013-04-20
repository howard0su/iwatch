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
#include "btstack/bluetooth.h"

#include <stdio.h>

extern tContext context;
extern tRectangle client_clip;

static uint8_t state;

#define STATE_X 10
#define STATE_Y 38

#define TITLE_X 10
#define TITLE_Y 68

#define PROGRESS_Y 80

static uint8_t bt_handler(uint8_t ev, uint16_t lparam, void* rparam)
{
  GrContextFontSet(&context, &g_sFontNova30b);

  switch(ev)
  {
  case AVRCP_EVENT_CONNECTED:
    avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);
    avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
    break;
  case AVRCP_EVENT_DISCONNECTED:
    break;
  case AVRCP_EVENT_TRACK_CHANGED:
    {
      switch(lparam)
      {
      case AVRCP_MEDIA_ATTRIBUTE_TITLE:
        {
          GrContextForegroundSet(&context, COLOR_BLACK);
          tRectangle rect = {TITLE_X, TITLE_Y, TITLE_X + 90, TITLE_Y + 40};
          GrRectFill(&context, &rect);
          GrContextForegroundSet(&context, COLOR_WHITE);

          GrStringDraw(&context, rparam, -1, TITLE_X, TITLE_Y, 0);
          break;
        }
        case AVRCP_MEDIA_ATTRIBUTE_DURATION:
        break;
      case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
        break;
      }
      break;
    }
  case AVRCP_EVENT_STATUS_CHANGED:
    {
      state = lparam;
      GrContextForegroundSet(&context, COLOR_BLACK);
      tRectangle rect = {STATE_X, STATE_Y, STATE_X + 90, STATE_Y + 40};
      GrRectFill(&context, &rect);
      GrContextForegroundSet(&context, COLOR_WHITE);
      switch(state)
      {
      case AVRCP_PLAY_STATUS_ERROR:
        GrStringDraw(&context, "ERROR", -1, STATE_X, STATE_Y, 0);
        window_button(KEY_UP, NULL);
        window_button(KEY_DOWN, NULL);
        window_button(KEY_ENTER, NULL);
        break;
      case AVRCP_PLAY_STATUS_STOPPED:
        GrStringDraw(&context, "STOP", -1, STATE_X, STATE_Y, 0);
        window_button(KEY_UP, "PLAY");
        window_button(KEY_DOWN, "NEXT");
        window_button(KEY_ENTER, "PREV");
        break;
      case AVRCP_PLAY_STATUS_PLAYING:
        GrStringDraw(&context, "PLAY", -1, STATE_X, STATE_Y, 0);
        window_button(KEY_UP, "PAUSE");
        window_button(KEY_DOWN, "NEXT");
        window_button(KEY_ENTER, "PREV");
        break;
      case AVRCP_PLAY_STATUS_PAUSED:
        GrStringDraw(&context, "PAUSE", -1, STATE_X, STATE_Y, 0);
        window_button(KEY_UP, "PLAY");
        window_button(KEY_DOWN, "NEXT");
        window_button(KEY_ENTER, "PREV");
        break;
      }
      break;
    }
  }

  printf("state = %d\n", (uint16_t)state);
  GrFlush(&context);

  return 1;
}

uint8_t control_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
    {
      GrContextForegroundSet(&context, COLOR_BLACK);
      GrRectFill(&context, &client_clip);

      avrcp_register_handler(bt_handler);
      state = AVRCP_PLAY_STATUS_ERROR;

      if (!avctp_connected() && bluetooth_paired())
      {
        avctp_connect(*bluetooth_paired_addr());
      }
      else
      {
        avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);
        avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
      }
      // call update
      GrFlush(&context);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      if (lparam == KEY_UP)
      {
        if (state == AVRCP_PLAY_STATUS_PLAYING)
        {
          avctp_send_passthrough(PAUSE_OP);
        }
        else
        {
          avctp_send_passthrough(PLAY_OP);
        }
      }
      else if (lparam == KEY_DOWN)
      {
        avctp_send_passthrough(FORWARD_OP);
      }
      else if (lparam == KEY_ENTER)
      {
        avctp_send_passthrough(BACKWARD_OP);
      }
      break;
    }
  case EVENT_WINDOW_CLOSING:
    {
      avrcp_register_handler(NULL);
      avctp_disconnect();
      break;
    }
  default:
    return 0;
  }
  return 1;
}