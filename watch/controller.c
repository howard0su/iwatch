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
#include <string.h>

static uint8_t state;

#define STATE_X 10
#define STATE_Y 38

#define TITLE_X 10
#define TITLE_Y 68

#define PROGRESS_Y 80

static char title[32];
static char artist[32];
static uint16_t length;
static uint16_t position;

static void OnDraw(tContext *pContext)
{
  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &client_clip);

  GrContextFontSet(pContext, &g_sFontNova28b);
  // draw length
  if (length != 0)
  {
    uint8_t times[3];
    times[2] = position % 60;
    times[1] = (position / 60) % 60;
    times[0] = position / 3600;

    window_drawtime(pContext, 65, times, 0);
  }

    // draw title
    GrContextFontSet(pContext, &g_sFontNova12);
    GrContextForegroundSet(pContext, ClrWhite);
    GrContextBackgroundSet(pContext, ClrBlack);
    GrStringDraw(pContext, title, -1, 25, 83, 0);

    switch(state)
    {
    case AVRCP_PLAY_STATUS_ERROR:
      window_button(pContext, KEY_UP, NULL);
      window_button(pContext, KEY_DOWN, NULL);
      window_button(pContext, KEY_ENTER, NULL);
      break;
    case AVRCP_PLAY_STATUS_STOPPED:
      window_button(pContext, KEY_UP, "PLAY");
      window_button(pContext, KEY_DOWN, "NEXT");
      window_button(pContext, KEY_ENTER, "PREV");
      break;
    case AVRCP_PLAY_STATUS_PLAYING:
      window_button(pContext, KEY_UP, "PAUSE");
      window_button(pContext, KEY_DOWN, "NEXT");
      window_button(pContext, KEY_ENTER, "PREV");
      break;
    case AVRCP_PLAY_STATUS_PAUSED:
      window_button(pContext, KEY_UP, "PLAY");
      window_button(pContext, KEY_DOWN, "NEXT");
      window_button(pContext, KEY_ENTER, "PREV");
      break;
    }

}
static uint8_t bt_handler(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case AVRCP_EVENT_CONNECTED:
    avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);
    avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
    avrcp_get_playstatus();
    return 1;
  case AVRCP_EVENT_DISCONNECTED:

    break;
  case AVRCP_EVENT_TRACK_CHANGED:
    {
      switch(lparam)
      {
      case AVRCP_MEDIA_ATTRIBUTE_TITLE:
        {
          strncpy(title, rparam, sizeof(title) - 1);
          break;
        }
      case AVRCP_MEDIA_ATTRIBUTE_DURATION:
        break;
      case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
        {
          strncpy(artist, rparam, sizeof(artist) - 1);
          break;
        }
      }
      break;
    }
  case AVRCP_EVENT_STATUS_CHANGED:
    {
      state = lparam;
      break;
    }
  }

  printf("state = %d\n", (uint16_t)state);
  window_invalid(NULL);

  return 1;
}

uint8_t control_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
    {
      avrcp_register_handler(bt_handler);
      state = AVRCP_PLAY_STATUS_ERROR;

      strcpy(title, "Connecting");
      if (!avctp_connected())
      {
        if (bluetooth_paired())
        {
          avctp_connect(*bluetooth_paired_addr());
        }
        else
        {
          window_notify("No Bluetooth Device Paired", NOTIFY_OK, NULL);
          return 1;
        }
      }
      else
      {
        avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);
        avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
        avrcp_get_playstatus();
      }

      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      OnDraw((tContext*)rparam);
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