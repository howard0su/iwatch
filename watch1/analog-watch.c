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
#include "cordic.h"
#include <stdio.h>
#include <string.h>
#include "memory.h"
/*
* This implement the digit watch
* Wake up every 1 second and update the watch
* If in 10 minutes, no key or other things
* if get system key in non-suspend state, post event to system.
*/

#define CENTER_X (pContext->pDisplay->usWidth/2)
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
#define CENTER_Y 91
#define MIN_HAND_LEN 40
#define HOUR_HAND_LEN 26
#else
#define CENTER_Y (pContext->pDisplay->usHeight/2)
#define MIN_HAND_LEN 50
#define HOUR_HAND_LEN 36
#endif


#define _hour d.analog.hour
#define _minute d.analog.minute
#define _sec d.analog.sec
static uint8_t selection;

typedef void (*draw_function)(tContext *pContext);

int centerX[4] = {71, 122,  71, 21};
int centerY[4] = {41,  90, 144, 90};	
static void drawFace0(tContext *pContext)
{
  int cos_val, sin_val;
  int sx, sy, ex, ey;

  for(int angle = 0; angle < 359; angle +=30)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    ex = CENTER_X + cordic_multipy(sin_val, CENTER_X - 20);
    ey = CENTER_Y - cordic_multipy(cos_val, CENTER_X - 20);
    sx = CENTER_X + cordic_multipy(sin_val, CENTER_X - 3);
    sy = CENTER_Y - cordic_multipy(cos_val, CENTER_X - 3);

    //tRect rect = {sx, sy, ex, ey};
    GrLineFill(pContext, sx, sy, ex, ey, 5);
  }
}

static void drawFace3(tContext *pContext)
{
  int cos_val, sin_val;
  int sx, sy, ex, ey;

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
  int x, y;
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 65);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 66);

  for(int angle = 0; angle < 359; angle += 30)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + cordic_multipy(sin_val, 60);
    y = CENTER_Y - cordic_multipy(cos_val, 60);

#if 0
    if (angle % 90 == 0)
    {
      GrCircleFill(pContext, x, y, 4);
    }
    else
#endif
    {
      GrCircleFill(pContext, x, y, 2);
    }
  }
}

static void drawFace7(tContext *pContext)
{
  int cos_val, sin_val;
  int x, y;
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 62);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 63);

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + cordic_multipy(sin_val, 57);
    y = CENTER_Y - cordic_multipy(cos_val, 57);

    GrCircleFill(pContext, x, y, 1);
  }
}

static void drawFace4(tContext *pContext)
{
  int cos_val, sin_val;
  int sx, sy, ex, ey;

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
  int x, y;

  for(int angle = 0; angle < 359; angle += 6)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + ((69 * (sin_val >> 8)) >> 7);
    y = CENTER_Y - ((69 * (cos_val >> 8)) >> 7);

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

static void drawFace5(tContext *pContext)
{
  int cos_val, sin_val;
  int x, y;

  GrContextFontSet(pContext, &g_sFontNimbus30);

  for(int angle = 60; angle <= 360; angle += 60)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + cordic_multipy(sin_val, 60);
    y = CENTER_Y - cordic_multipy(cos_val, 60);

    char buf[30];
    sprintf(buf, "%d", angle/30);
    GrStringDrawCentered(pContext, buf, -1, x, y, 0);
  }

  for(int angle = 30; angle <= 360; angle += 60)
  {
    cordic_sincos(angle, 13, &sin_val, &cos_val);
    x = CENTER_X + cordic_multipy(sin_val, 60);
    y = CENTER_Y - cordic_multipy(cos_val, 60);

    GrCircleFill(pContext, x, y, 3);
  }
}

static void drawFace10(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrWhite);
	const tRectangle rect = {13 , 34, 130, 151};	
	GrRectFillRound(pContext, &rect, 5);
	GrContextForegroundSet(pContext, ClrBlack);
	const tRectangle rect1 = {15 , 36, 128, 149};	
	GrRectFillRound(pContext, &rect1, 5);
	
	GrContextForegroundSet(pContext, ClrWhite);
	for(int angle = 0; angle < 4; angle ++)
	{
		GrCircleFill(pContext, centerX[angle], centerY[angle], 3);
	}
}

static void drawFace11(tContext *pContext)
{
	int cos_val, sin_val;  	
  	int sx, sy, ex, ey,x,y;
	
	GrContextForegroundSet(pContext, ClrWhite);
	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 67);
  	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 66);
  	
  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 57);
  	
  	for(int angle = 0; angle < 359; angle += 30)
  	{
  		cordic_sincos(angle, 13, &sin_val, &cos_val);
  		x = CENTER_X + ((67 * (sin_val >> 8)) >> 7);
    		y = CENTER_Y - ((67 * (cos_val >> 8)) >> 7);
    		
    		if (angle % 90 == 0)
    		{
    			GrCircleFill(pContext, x, y, 3);
    		}
    		else
    		{
    			const tRectangle rect = {x-2 , y-3, x+2, y+2};	
			GrRectFillRound(pContext, &rect, 0);
    		}	
		GrContextForegroundSet(pContext, ClrBlack);	
		sx = CENTER_X + ((60 * (sin_val >> 8)) >> 7);
      		sy = CENTER_Y - ((60 * (cos_val >> 8)) >> 7);
      			
      		ex = CENTER_X + ((50 * (sin_val >> 8)) >> 7);
      		ey = CENTER_Y - ((50 * (cos_val >> 8)) >> 7);
      			
      		GrLineDraw(pContext, sx, sy, ex, ey);    		
    		GrContextForegroundSet(pContext, ClrWhite);
  	}
}

static void drawFace12(tContext *pContext)
{
	int cos_val, sin_val;
  	int sx, sy, ex,ey;
  	GrContextForegroundSet(pContext, ClrWhite);
	for(int angle = 60; angle <= 360; angle += 60)
	{//draw dot
		cordic_sincos(angle, 13, &sin_val, &cos_val);
		sx = CENTER_X + ((66 * (sin_val >> 8)) >> 7);
    		sy = CENTER_Y - ((66 * (cos_val >> 8)) >> 7);
    		const tRectangle rect = {sx , sy, sx+1, sy+1};	
		GrRectFillRound(pContext, &rect, 0);
	}
	
	for(int angle = 30; angle <= 360; angle += 60)
	{	//draw line
		cordic_sincos(angle, 13, &sin_val, &cos_val);
		sx = CENTER_X + ((68 * (sin_val >> 8)) >> 7);
      		sy = CENTER_Y - ((68 * (cos_val >> 8)) >> 7);
      			
      		ex = CENTER_X + ((57 * (sin_val >> 8)) >> 7);
      		ey = CENTER_Y - ((57 * (cos_val >> 8)) >> 7);
      		
      		GrLineDraw(pContext, sx, sy, ex, ey);    		    		
    		GrLineDraw(pContext, sx, sy+1, ex, ey+1);    		
    		GrContextForegroundSet(pContext, ClrWhite);      		    		    		
	}
}

static void drawFace13(tContext *pContext)
{
	int cos_val, sin_val;  	
  	int x,y;
	
	GrContextForegroundSet(pContext, ClrWhite);	
	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 59);
  	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 58);	
  	
  	for(int angle = 0; angle < 359; angle += 30)
  	{
  		cordic_sincos(angle, 13, &sin_val, &cos_val);
		x = CENTER_X + ((51 * (sin_val >> 8)) >> 7);
    		y = CENTER_Y - ((51 * (cos_val >> 8)) >> 7);
    		const tRectangle rect = {x , y, x+1, y+1};	
		GrRectFillRound(pContext, &rect, 0);
  	}
}

static void drawFace14(tContext *pContext)
{
	int cos_val, sin_val;
  	int sx, sy, ex,ey;
  	
  	GrContextForegroundSet(pContext, ClrWhite);
	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 60);
	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 59);
  	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 58);	  	
  	
  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 44);
  	GrContextForegroundSet(pContext, ClrBlack);	
  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 24);
  	
  	GrContextForegroundSet(pContext, ClrWhite);
  	for(int angle = 30; angle <= 360; angle += 30)
	{	//draw line
		cordic_sincos(angle, 13, &sin_val, &cos_val);
		sx = CENTER_X + ((57 * (sin_val >> 8)) >> 7);
      		sy = CENTER_Y - ((57 * (cos_val >> 8)) >> 7);
      			
      		ex = CENTER_X + ((44 * (sin_val >> 8)) >> 7);
      		ey = CENTER_Y - ((44 * (cos_val >> 8)) >> 7);
      		
      		GrLineDraw(pContext, sx, sy, ex, ey);    		    		    		
    		GrContextForegroundSet(pContext, ClrWhite);      		    		    		
	}
  	
  	
  		
}

static void drawFace15(tContext *pContext)
{
  	int cos_val, sin_val;
  	int sx, sy, ex, ey;
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrContextFontSet(pContext, &g_sFontMetroid20);	
  	
  	for(int angle = 45; angle <= 360; angle += 45)
  	{
		cordic_sincos(angle, 13, &sin_val, &cos_val);

    		if(angle%90 ==0)
    		{	
    			sx = CENTER_X + cordic_multipy(sin_val, 58);
    			sy = CENTER_Y - cordic_multipy(cos_val, 58);  		
    			char buf[30];
    			sprintf(buf, "%d", angle/30);
    			GrStringDrawCentered(pContext, buf, -1, sx, sy, 0);    		
    		}
    		else
    		{
			sx = CENTER_X + ((60 * (sin_val >> 8)) >> 7);
      			sy = CENTER_Y - ((60 * (cos_val >> 8)) >> 7);      			
      			ex = CENTER_X + ((50 * (sin_val >> 8)) >> 7);
      			ey = CENTER_Y - ((50 * (cos_val >> 8)) >> 7);
      			
      			GrLineFill(pContext, sx, sy, ex, ey, 3);    		      			

    		}
  	}
  	
}

static void drawFace16(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrWhite);
	const tRectangle rect = {12 , 32, 131, 151};	
	GrRectFillRound(pContext, &rect, 8);	
	GrContextForegroundSet(pContext, ClrBlack);
}

static void drawFace17(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrWhite);
	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 58);
  	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 50);
  	GrCircleDraw(pContext, CENTER_X, CENTER_Y, 21);
  	
  	
  		
}

static void drawFace18(tContext *pContext)
{
	int cos_val, sin_val;  	
  	int sx, sy, ex, ey;
  	
	GrContextForegroundSet(pContext, ClrWhite);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrBlack);	
	for(int angle = 90; angle <= 360; angle += 90)
	{
		cordic_sincos(angle, 13, &sin_val, &cos_val);
		sx = CENTER_X + ((74 * (sin_val >> 8)) >> 7);
      		sy = CENTER_Y - ((74 * (cos_val >> 8)) >> 7);
      			
      		ex = CENTER_X + ((55 * (sin_val >> 8)) >> 7);
      		ey = CENTER_Y - ((55 * (cos_val >> 8)) >> 7);		
      		
      		GrLineFill(pContext, sx, sy, ex, ey, 2);
	}
	
}
// design 3, hand
static void drawHand0(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  int x, y;

  // minute hand = length = 70
  angle = _minute * 6+ _sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);

  // hour hand 45
  angle = _hour * 30 + _minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  x = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  y = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineFill(pContext, CENTER_X, CENTER_Y,  x, y, 2);
}

static void drawHand1(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  int sx, sy, ex, ey;

  // draw the circle
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 3);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 9);
  GrCircleDraw(pContext, CENTER_X, CENTER_Y, 10);

  // minute hand = length = 70
  angle = _minute * 6+ _sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  ey = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  sx = CENTER_X + ((14 * (sin_val >> 8)) >> 7);
  sy = CENTER_Y - ((14 * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineFill(pContext, sx, sy,  ex, ey, 4);

  // hour hand 45
  angle = _hour * 30 + _minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  ey = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  sx = CENTER_X + ((14 * (sin_val >> 8)) >> 7);
  sy = CENTER_Y - ((14 * (cos_val >> 8)) >> 7);
  GrContextForegroundSet(pContext, ClrWhite);
  GrLineFill(pContext, sx, sy,  ex, ey, 3);
}

static void drawHand2(tContext *pContext)
{
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 12);

  drawHand0(pContext);
}

static void drawHand4(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  int sx, sy, ex, ey, mx, my;

  // degree is caculated by 
  // arctan(35/7)=?deg

  // hour hand 45
  angle = _hour * 30 + _minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  mx = CENTER_X + cordic_multipy(sin_val, 37);
  my = CENTER_Y - cordic_multipy(cos_val, 37);

  angle = angle - 79;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  sx = CENTER_X + cordic_multipy(sin_val, 7);
  sy = CENTER_Y - cordic_multipy(cos_val, 7);

  angle = angle + 79*2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + cordic_multipy(sin_val, 7);
  ey = CENTER_Y - cordic_multipy(cos_val, 7);
  // draw a filled triagle
  GrTriagleFill(pContext, 
    ex, ey,
    sx, sy,
    mx, my
    );

  // draw a black
  GrContextForegroundSet(pContext, ClrBlack);
  GrCircleFill(pContext, CENTER_X, CENTER_Y, 8);
  GrContextForegroundSet(pContext, ClrWhite);

  // minute hand
  angle = _minute * 6+ _sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  mx = CENTER_X + cordic_multipy(sin_val, 59);
  my = CENTER_Y - cordic_multipy(cos_val, 59);

  angle = angle - 84;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  sx = CENTER_X + cordic_multipy(sin_val, 7);
  sy = CENTER_Y - cordic_multipy(cos_val, 7);

  angle = angle + 84*2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + cordic_multipy(sin_val, 7);
  ey = CENTER_Y - cordic_multipy(cos_val, 7);
  // draw a filled triagle
  GrTriagleFill(pContext, 
    ex, ey,
    sx, sy,
    mx, my
    );

  GrCircleFill(pContext, CENTER_X, CENTER_Y, 7);
}



static void drawHand5(tContext *pContext)
{
  int cos_val, sin_val;
  int angle;
  int sx, sy, ex, ey, mx, my;

  // degree is caculated by 
  // arctan(35/7)=?deg

  // hour hand 45
  angle = _hour * 30 + _minute / 2;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  mx = CENTER_X + cordic_multipy(sin_val, 53);
  my = CENTER_Y - cordic_multipy(cos_val, 53);

  angle = angle - 150;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  sx = CENTER_X + cordic_multipy(sin_val, 13);
  sy = CENTER_Y - cordic_multipy(cos_val, 13);

  angle = angle + 300;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + cordic_multipy(sin_val, 13);
  ey = CENTER_Y - cordic_multipy(cos_val, 13);

  GrLineDraw(pContext, ex, ey, sx, sy);
  GrLineDraw(pContext, mx, my, sx, sy);
  GrLineDraw(pContext, ex, ey, mx, my);

  // minute hand
  angle = _minute * 6+ _sec /10;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  mx = CENTER_X + cordic_multipy(sin_val, 65);
  my = CENTER_Y - cordic_multipy(cos_val, 65);

  angle = angle - 150;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  sx = CENTER_X + cordic_multipy(sin_val, 12);
  sy = CENTER_Y - cordic_multipy(cos_val, 12);

  angle = angle + 300;
  cordic_sincos(angle, 13, &sin_val, &cos_val);
  ex = CENTER_X + cordic_multipy(sin_val, 12);
  ey = CENTER_Y - cordic_multipy(cos_val, 12);

  GrLineDraw(pContext, ex, ey, sx, sy);
  GrLineDraw(pContext, mx, my, sx, sy);
  GrLineDraw(pContext, ex, ey, mx, my);

  GrPixelDraw(pContext, CENTER_X, CENTER_Y);
}

static void drawHand10(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int x, y;	
  	
  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 3);
  	
  	// minute hand = length = 70
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	x = CENTER_X + ((MIN_HAND_LEN * (sin_val >> 8)) >> 7);
  	y = CENTER_Y - ((MIN_HAND_LEN * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);

  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	x = CENTER_X + ((HOUR_HAND_LEN * (sin_val >> 8)) >> 7);
  	y = CENTER_Y - ((HOUR_HAND_LEN * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrLineDraw(pContext, CENTER_X, CENTER_Y,  x, y);
}

static void drawHand11(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int sx, sy, ex, ey, mx, my;	
  	
  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 28);
  	my = CENTER_Y - cordic_multipy(cos_val, 28);

  	angle = angle - 79;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 6);
  	sy = CENTER_Y - cordic_multipy(cos_val, 6);

  	angle = angle + 79*2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 6);
  	ey = CENTER_Y - cordic_multipy(cos_val, 6);
  	// draw a filled triagle
  	GrContextForegroundSet(pContext, ClrBlack);
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	// draw a black
  	GrContextForegroundSet(pContext, ClrWhite);  	
  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 10);
  	GrContextForegroundSet(pContext, ClrBlack);

  	// minute hand
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 44);
  	my = CENTER_Y - cordic_multipy(cos_val, 44);

  	angle = angle - 84;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 6);
  	sy = CENTER_Y - cordic_multipy(cos_val, 6);

  	angle = angle + 84*2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 6);
  	ey = CENTER_Y - cordic_multipy(cos_val, 6);
  	// draw a filled triagle
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	GrCircleFill(pContext, CENTER_X, CENTER_Y, 7);  	
}

static void drawHand12(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int sx, sy, ex, ey, mx, my;

  	// degree is caculated by 
  	// arctan(35/7)=?deg

  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 36);
  	my = CENTER_Y - cordic_multipy(cos_val, 36);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 8);
  	sy = CENTER_Y - cordic_multipy(cos_val, 8);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 8);
  	ey = CENTER_Y - cordic_multipy(cos_val, 8);

	GrContextForegroundSet(pContext, ClrWhite);  	
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	// minute hand
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 50);
  	my = CENTER_Y - cordic_multipy(cos_val, 50);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 10);
  	sy = CENTER_Y - cordic_multipy(cos_val, 10);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 10);
  	ey = CENTER_Y - cordic_multipy(cos_val, 10);

	GrContextForegroundSet(pContext, ClrWhite);  	
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	GrPixelDraw(pContext, CENTER_X, CENTER_Y);	
}

static void drawHand13(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int x, y;

	//Draw min hand fist to prevent hour hand be coverd
  	// minute hand = length = 70
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	x = CENTER_X + ((44 * (sin_val >> 8)) >> 7);
  	y = CENTER_Y - ((44 * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrBlack);  	
  	GrLineFill(pContext, CENTER_X, CENTER_Y,  x, y, 3);	  	
  		
  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	x = CENTER_X + ((24 * (sin_val >> 8)) >> 7);
  	y = CENTER_Y - ((24 * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrLineFill(pContext, CENTER_X, CENTER_Y,  x, y, 3);	
  	

}

static void drawHand14(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int sx, sy, ex, ey, mx, my;

  	// degree is caculated by 
  	// arctan(35/7)=?deg

  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 15);
  	my = CENTER_Y - cordic_multipy(cos_val, 15);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 8);
  	sy = CENTER_Y - cordic_multipy(cos_val, 8);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 8);
  	ey = CENTER_Y - cordic_multipy(cos_val, 8);

	GrContextForegroundSet(pContext, ClrWhite);  	
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	// minute hand
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 26);
  	my = CENTER_Y - cordic_multipy(cos_val, 26);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 8);
  	sy = CENTER_Y - cordic_multipy(cos_val, 8);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 8);
  	ey = CENTER_Y - cordic_multipy(cos_val, 8);

	GrContextForegroundSet(pContext, ClrWhite);  	
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	GrPixelDraw(pContext, CENTER_X, CENTER_Y);		
}

static void drawHand15(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int sx, sy, ex, ey, mx, my;

  	// degree is caculated by 
  	// arctan(35/7)=?deg

  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 40);
  	my = CENTER_Y - cordic_multipy(cos_val, 40);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 10);
  	sy = CENTER_Y - cordic_multipy(cos_val, 10);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 10);
  	ey = CENTER_Y - cordic_multipy(cos_val, 10);

	GrContextForegroundSet(pContext, ClrBlack);  	
  	GrTriagleFill(pContext, 
    			ex, ey,
    			sx, sy,
    			mx, my );

  	// minute hand
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	mx = CENTER_X + cordic_multipy(sin_val, 56);
  	my = CENTER_Y - cordic_multipy(cos_val, 56);

  	angle = angle - 150;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + cordic_multipy(sin_val, 10);
  	sy = CENTER_Y - cordic_multipy(cos_val, 10);

  	angle = angle + 300;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + cordic_multipy(sin_val, 10);
  	ey = CENTER_Y - cordic_multipy(cos_val, 10);

	GrContextForegroundSet(pContext, ClrBlack);  	
  	GrLineFill(pContext, ex, ey, sx, sy,1);
  	GrLineFill(pContext, mx, my, sx, sy,1);
  	GrLineFill(pContext, ex, ey, mx, my,1);	
}

static void drawHand16(tContext *pContext)
{
  	int cos_val, sin_val;
  	int angle;
  	int sx, sy,ex,ey;

	//Draw min hand fist to prevent hour hand be coverd
  	// hour hand 45
  	angle = _hour * 30 + _minute / 2;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + ((20 * (sin_val >> 8)) >> 7);
  	sy = CENTER_Y - ((20 * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrLineFill(pContext, CENTER_X, CENTER_Y,  sx, sy, 2);

  	// minute hand = length = 70
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	sx = CENTER_X + ((22 * (sin_val >> 8)) >> 7);
  	sy = CENTER_Y - ((22 * (cos_val >> 8)) >> 7);
  	
  	angle = _minute * 6+ _sec /10;
  	cordic_sincos(angle, 13, &sin_val, &cos_val);
  	ex = CENTER_X + ((49 * (sin_val >> 8)) >> 7);
  	ey = CENTER_Y - ((49 * (cos_val >> 8)) >> 7);
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrLineFill(pContext, sx, sy,  ex, ey, 2);	  	  	

}

#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
struct clock_draw {
  draw_function faceDraw;
  draw_function handDraw;
}FaceSelections[] =
{
  {drawFace10, drawHand10},
  {drawFace11, drawHand11},
  {drawFace12, drawHand12},
  {drawFace13, drawHand0},
  {drawFace14, drawHand13},
  {drawFace15, drawHand14},
  {drawFace16, drawHand15},
  {drawFace17, drawHand16},
  {drawFace18, drawHand15},
};
#else
struct clock_draw {
  draw_function faceDraw;
  draw_function handDraw;
}FaceSelections[] =
{
  {drawFace5, drawHand4},
  {drawFace5, drawHand5},
  {drawFace5, drawHand0},
  {drawFace0, drawHand4},
  {drawFace0, drawHand5},
  {drawFace0, drawHand0},
  {drawFace6, drawHand4},
  {drawFace6, drawHand5},
  {drawFace6, drawHand0},
};
#endif
uint8_t analogclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&_hour, &_minute, &_sec);
    if (rparam == NULL)
      selection = window_readconfig()->analog_clock;
    else
      selection = (uint8_t)rparam - 0x1;
    if (selection > sizeof(FaceSelections)/sizeof(struct clock_draw) - 1)
      selection = sizeof(FaceSelections)/sizeof(struct clock_draw) - 1;

    rtc_enablechange(MINUTE_CHANGE);

    return 0x80;
  }
  else if (ev == EVENT_WINDOW_ACTIVE)
  {
    rtc_readtime(&_hour, &_minute, &_sec);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    tContext *pContext = (tContext*)rparam;
    GrContextForegroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	GrRectFill(pContext, &client_clip);
#else    
    GrRectFill(pContext, &fullscreen_clip);
#endif
    GrContextForegroundSet(pContext, ClrWhite);

    FaceSelections[selection].faceDraw(pContext);
    FaceSelections[selection].handDraw(pContext);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    _hour = dt->hour;
    _minute = dt->minute;
    _sec = dt->second;
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

    window_readconfig()->default_clock = 0;
    if (selection != window_readconfig()->analog_clock)
    {
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
