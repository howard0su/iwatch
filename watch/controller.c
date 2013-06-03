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

  GrContextFontSet(pContext, &g_sFontNova38b);
  // draw length
  if (position != 0)
  {
    uint8_t times[3];
    uint16_t left = position;
    times[2] = left % 60;
    times[1] = (left / 60) % 60;
    times[0] = left / 3600;

    GrContextForegroundSet(pContext, ClrWhite);
    tRectangle rect = {12, 24, 132, 60};
    GrRectFillRound(pContext, &rect, 3);

    GrContextForegroundSet(pContext, ClrBlack);
    window_drawtime(pContext, 24, times, 0);
#if 0

    // draw balls
    long r;
    long startx = -130 + (position * 143/ length);
    for(int i = 0; i < 11; i++)
    {
      r = i/2 + 1;
      GrCircleFill(pContext, startx, 22, r);
      startx += r * 2 + 3;
    }
    for(int i = 10; i >0; i--)
    {
      r = i/2 + 1;
      GrCircleFill(pContext, startx, 22, r);
      startx += r * 2 + 3;
    }
#endif
  }

  // draw title
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova16b);
  GrStringDraw(pContext, title, -1, 12, 118, 0);

  GrContextFontSet(pContext, &g_sFontNova16);
  GrStringDraw(pContext, artist, -1, 12, 135, 0);
#if 0
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
#endif
}


uint8_t initing;

static uint8_t bt_handler(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case AVRCP_EVENT_CONNECTED:
    {
      if (initing)
      {
        strcpy(title, "Connected");
        avrcp_get_playstatus();
      }
      break;
    }
  case AVRCP_EVENT_DISCONNECTED:
    break;
  case AVRCP_EVENT_TRACK_CHANGED:
    {
      if (initing)
      {
        avrcp_enable_notification(AVRCP_EVENT_PLAYBACK_POS_CHANGED);
        initing = 0;
      }
      position = 0;
      avrcp_get_attributes(0);
      break;
    }
  case AVRCP_EVENT_ATTRIBUTE:
    {
      switch(lparam)
      {
      case AVRCP_MEDIA_ATTRIBUTE_TITLE:
        {
          strncpy(title, rparam, sizeof(title) - 1);
          break;
        }
      case AVRCP_MEDIA_ATTRIBUTE_DURATION:
        {
          break;
        }
      case AVRCP_MEDIA_ATTRIBUTE_ARTIST:
        {
          sprintf(artist, "by %s", (char*)rparam);
          avrcp_get_playstatus();
          break;
        }
      }
      break;
    }
  case AVRCP_EVENT_STATUS_CHANGED:
    {
      if (initing)
      {
        avrcp_enable_notification(AVRCP_EVENT_TRACK_CHANGED);
      }

      state = lparam;
      break;
    }
  case AVRCP_EVENT_LENGTH:
    {
      if (initing)
      {
        avrcp_enable_notification(AVRCP_EVENT_STATUS_CHANGED);
      }
      length = (uint16_t)rparam;
      break;
    }
  case AVRCP_EVENT_STATUS:
    {
      state = lparam;
      position = (uint16_t)rparam;
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
      initing = 1;
      strcpy(title, "Connecting");
      if (!avctp_connected())
      {
        if (bluetooth_paired())
        {
          avctp_connect(*bluetooth_paired_addr());
        }
        else
        {
          window_notify("ERROR", "No Bluetooth Device Paired", NOTIFY_OK, 0);
          return 1;
        }
      }
      else
      {
        //avrcp_get_playstatus();
        strcpy(title, "Connected");
        avrcp_get_playstatus();
      }
      window_timer(CLOCK_SECOND);
      break;
    }
  case PROCESS_EVENT_TIMER:
    {
      tRectangle rect = {12, 24, 144, 88};
      if (position < length && state == 1)
        position++;

      window_invalid(&rect);
      break;
    }
  case EVENT_NOTIFY_RESULT:
    window_close();
    break;
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
      window_timer(0);
      break;
    }
  default:
    return 0;
  }
  return 1;
}