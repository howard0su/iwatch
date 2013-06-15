/****************************************************************
*  Description: Implementation for Analog watch Window
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
#include "rtc.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "cordic.h"
#include <stdio.h>
#include <string.h>

/*
* This implement the digit watch
* Wake up every 1 second and update the watch
* If in 10 minutes, no key or other things
* if get system key in non-suspend state, post event to system.
*/
#define CENTER_X LCD_X_SIZE/2
#define CENTER_Y (LCD_Y_SIZE-16)/2 + 16

#define MIN_HAND_LEN 50
#define HOUR_HAND_LEN 36

static uint8_t hour, minute, sec;
static uint8_t selection;
typedef void (*draw_function)(tContext *pContext);

static void drawFace0(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t sx, sy, ex, ey;

  for(int angle = 0; angle < 359; angle +=30)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    ex = CENTER_X + ((64 * (sin_val >> 8)) >> 7);
    ey = CENTER_Y - ((64 * (cos_val >> 8)) >> 7);
    sx = CENTER_X + ((52 * (sin_val >> 8)) >> 7);
    sy = CENTER_Y - ((52 * (cos_val >> 8)) >> 7);

    GrLineDraw(pContext, sx, sy, ex, ey);
  }
}

static void drawFace3(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t sx, sy, ex, ey;

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    sx = CENTER_X + ((52 * (sin_val >> 8)) >> 7);
    sy = CENTER_Y - ((52 * (cos_val >> 8)) >> 7);

    if (angle % 30 == 0)
    {
      ex = CENTER_X + ((64 * (sin_val >> 8)) >> 7);
      ey = CENTER_Y - ((64 * (cos_val >> 8)) >> 7);

      GrLineDraw(pContext, sx, sy, ex, ey);
    }
    else
    {
      GrCircleFill(pContext, sx, sy, 2);
    }
  }
}

static void drawFace6(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t x, y;
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 62);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 63);

  for(int angle = 0; angle < 359; angle += 30)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + ((57 * (sin_val >> 8)) >> 7);
    y = CENTER_Y - ((57 * (cos_val >> 8)) >> 7);

    if (angle % 90 == 0)
    {
      GrCircleFill(pContext, x, y, 4);
    }
    else
    {
      GrCircleFill(pContext, x, y, 2);
    }
  }
}

static void drawFace7(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t x, y;
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 62);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 63);

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + ((57 * (sin_val >> 8)) >> 7);
    y = CENTER_Y - ((57 * (cos_val >> 8)) >> 7);

    GrCircleFill(pContext, x, y, 1);
  }
}

static void drawFace4(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t sx, sy, ex, ey;

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    sx = CENTER_X + ((64 * (sin_val >> 8)) >> 7);
    sy = CENTER_Y - ((64 * (cos_val >> 8)) >> 7);

    if (angle % 30 == 0)
    {
      ex = CENTER_X + ((53 * (sin_val >> 8)) >> 7);
      ey = CENTER_Y - ((53 * (cos_val >> 8)) >> 7);

      GrLineDraw(pContext, sx, sy, ex, ey);
    }
    else
    {
      GrCircleFill(pContext, sx, sy, 2);
    }
  }
}

static void drawFace1(tContext *pContext)
{
  int cos_val, sin_val;
  uint8_t x, y;

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + ((57 * (sin_val >> 8)) >> 7);
    y = CENTER_Y - ((57 * (cos_val >> 8)) >> 7);

    if (angle % 30 == 0)
    {
      GrCircleFill(pContext, x, y, 3);
    }
    else
    {
      GrCircleFill(pContext, x, y, 1);
    }
  }
}

// design 3, hand
static void drawHand0(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  uint16_t x, y;

  // minute hand = length = 70
  angle = minute * 6+ sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);

  // hour hand 45
  angle = hour * 30 + minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);
}

static void drawHand1(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  uint16_t sx, sy, ex, ey;

  // draw the circle
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 3);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 9);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 10);

  // minute hand = length = 70
  angle = minute * 6+ sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  ey = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  sx = CENTER_X + ((14 * (sin_val >> 8)) >> 7);
  sy = CENTER_Y - ((14 * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, sx, sy, ex, ey);

  // hour hand 45
  angle = hour * 30 + minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  ey = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  sx = CENTER_X + ((14 * (sin_val >> 8)) >> 7);
  sy = CENTER_Y - ((14 * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, sx, sy,  ex, ey);
}

static void drawHand2(tContext *pContext)
{
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 12);

  drawHand0(pContext);
}

struct clock_draw {
  draw_function faceDraw;
  draw_function handDraw;
}FaceSelections[] =
{
  {drawFace0, drawHand0},
  {drawFace1, drawHand1},
  {drawFace3, drawHand2},
  {drawFace4, drawHand0},
  {drawFace6, drawHand1},
  {drawFace7, drawHand2},
};

uint8_t analogclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&hour, &minute, &sec);
    if (rparam == NULL)
      selection = window_readconfig()->analog_clock;
    else
      selection = (uint8_t)rparam - 0x1;
    rtc_enablechange(MINUTE_CHANGE);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    tContext *pContext = (tContext*)rparam;
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &client_clip);

    GrContextForegroundSet(pContext, ClrWhite);

    FaceSelections[selection].faceDraw(pContext);
    FaceSelections[selection].handDraw(pContext);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    hour = dt->hour;
    minute = dt->minute;
    sec = dt->second;
    window_invalid(NULL);
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_DOWN)
    {
      selection += 0x1;
      if (selection > sizeof(FaceSelections)/sizeof(struct clock_draw) - 1)
      {
        selection = 0x00;
      }
      window_invalid(NULL);
    }
    else if (lparam == KEY_UP)
    {
      selection -= 0x1;
      if (selection == 0xff)
      {
        selection = sizeof(FaceSelections)/sizeof(struct clock_draw) - 1;
      }
      window_invalid(NULL);
    }
  }
  else if (ev == EVENT_WINDOW_CLOSING)
  {
    rtc_enablechange(0);

    if (selection != window_readconfig()->analog_clock)
    {
      window_readconfig()->default_clock = 0;
      window_readconfig()->analog_clock = selection;
      window_writeconfig();
    }
  }
  else
  {
    return 0;
  }

  return 1;
}