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
#include "math.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include "memory.h"

#include <stdio.h> // for sprintf
#include <string.h>

#define _hour0 d.digit.hour0
#define _minute d.digit.minute
static uint8_t _selection;

typedef void (*draw_function)(tContext *pContext);

void adjustAMPM(uint8_t hour, uint8_t *outhour, uint8_t *ispm)
{
  if (hour > 12) // 12 - 23
  {
    *ispm = 1;
    *outhour -= 12;
  }
  else if (hour == 12)
  {
    *ispm = 1;
  }
  else if (hour == 0)
  {
    *outhour = 12;
  }
}

static void drawClock0(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ispm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus38);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH/2, 60, 0);

  int width = GrStringWidthGet(pContext, buf, -1);
  GrContextFontSet(pContext, &g_sFontGothic18);
  if (ispm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, LCD_WIDTH/2+width/2 - 
    GrStringWidthGet(pContext, buf, 2) - 2
    , 84, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  sprintf(buf, "%s %d, %d", toMonthName(month, 1), day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH/2, 137, 0);
}

static void drawClock1(tContext *pContext)
{
  uint8_t ispm = 0;
  uint8_t hour = _hour0;
  char buf[20];

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus52);

  sprintf(buf, "%02d", hour);
  GrStringDrawCentered(pContext, buf, 2, LCD_WIDTH/ 2, 60, 0);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus52);
  sprintf(buf, "%02d", _minute);
  GrStringDrawCentered(pContext, buf, 2, LCD_WIDTH / 2, 100, 0);

  GrContextFontSet(pContext, &g_sFontGothic18b);
  if (ispm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDrawCentered(pContext, buf, 2, LCD_WIDTH / 2, 132, 0);
}

static void drawClock2(tContext *pContext)
{
  uint8_t ispm = 0;
  uint8_t hour = _hour0;
  char buf[20];
  const char* buffer;

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, &g_sFontNimbus34);
  buffer = toEnglish(hour, buf);
  GrStringDraw(pContext, buffer, -1, 5, 58, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  buffer = toEnglish(_minute, buf);
  GrStringDraw(pContext, buffer, -1, 5, 88, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  if (ispm)
  {
    GrStringDraw(pContext, "In the PM", -1, 5, 120, 0);
  }
  else
  {
    GrStringDraw(pContext, "In the AM", -1, 5, 120, 0);
  }
}


static void drawClock3(tContext *pContext)
{
  uint8_t ispm = 0;
  uint8_t hour = _hour0;
  char buf[20];
  const char* buffer;

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, &g_sFontNimbus34);

  buffer = toEnglish(hour, buf);
  tRectangle rect = {5, 32, LCD_WIDTH - 5, 80};
  GrRectFill(pContext, &rect);
  GrContextForegroundSet(pContext, ClrBlack);
  GrStringDrawCentered(pContext, buffer, -1, LCD_WIDTH/ 2, 56, 0);
  GrContextForegroundSet(pContext, ClrWhite);
  
  GrContextFontSet(pContext, &g_sFontGothic24b);
  buffer = toEnglish(_minute, buf);
  GrStringDrawCentered(pContext, buffer, -1, LCD_WIDTH / 2, 95, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  if (ispm)
  {
    GrStringDrawCentered(pContext, "In the PM", -1, LCD_WIDTH / 2, 120, 0);
  }
  else
  {
    GrStringDrawCentered(pContext, "In the AM", -1, LCD_WIDTH / 2, 120, 0);
  }
}

static void drawClock8(tContext *pContext)
{
    GrContextForegroundSet(pContext, ClrWhite);
    GrRectFill(pContext, &fullscreen_clip);
    GrContextForegroundSet(pContext, ClrBlack);

    drawClock0(pContext);
}

static void drawClock4(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus50);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 68, 0);

  GrContextFontSet(pContext, &g_sFontGothic18b);
  sprintf(buf, "%s %d, %d", toMonthName(month, 1), day, year);
  tRectangle rect = {6, 90, LCD_WIDTH-6, 120};
  GrRectFillRound(pContext, &rect, 8);
  GrContextForegroundSet(pContext, ClrBlack);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 105, 0);
}

static void drawClock5(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ispm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus46);

  sprintf(buf, "%02d:%02d", hour, _minute);  
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 70, 0);

  int width = GrStringWidthGet(pContext, buf, -1);
  GrContextFontSet(pContext, &g_sFontGothic18b);
  if (ispm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, LCD_WIDTH/2+width/2 - 
    GrStringWidthGet(pContext, buf, 2) - 2
    , 92, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  sprintf(buf, "%02d-%02d-%02d", month, day, year - 2000);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 143, 0);
}

static void drawClock6(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  tRectangle rect = {30, 26, LCD_WIDTH - 30, LCD_Y_SIZE - 37};
  GrRectFillRound(pContext, &rect, 8);
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus52);
  sprintf(buf, "%02d", hour);
  GrStringDrawCentered(pContext, buf, 1, LCD_WIDTH / 4 + 20, 54, 0);
  GrStringDrawCentered(pContext, buf + 1, 1, LCD_WIDTH / 2 + LCD_WIDTH/4 - 20, 54, 0);

  sprintf(buf, "%02d", _minute);
  GrStringDrawCentered(pContext, buf, 1, LCD_WIDTH / 4 + 20, 100, 0);
  GrStringDrawCentered(pContext, buf + 1, 1, LCD_WIDTH / 2 + LCD_WIDTH/4 - 20, 100, 0);

  GrContextFontSet(pContext, &g_sFontGothic18);
  GrContextForegroundSet(pContext, ClrWhite);
  sprintf(buf, "%s %d, %d", toMonthName(month, 1), day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 150, 0);
}


static void drawClock7(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  uint8_t ispm = 0;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  adjustAMPM(hour, &hour, &ispm);

  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus52);

  sprintf(buf, "%02d:%02d", hour, _minute);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 70, 0);

  int width = GrStringWidthGet(pContext, buf, -1);
  GrContextFontSet(pContext, &g_sFontGothic18b);
  if (ispm) buf[0] = 'P';
    else buf[0] = 'A';
  buf[1] = 'M';
  GrStringDraw(pContext, buf, 2, LCD_WIDTH/2+width/2 - 
    GrStringWidthGet(pContext, buf, 2) - 2
    , 95, 0);


  GrContextFontSet(pContext, &g_sFontGothic18);
  sprintf(buf, "%s %d, %d", toMonthName(month, 1), day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 35, 0);
}

static void drawClock9(tContext *pContext)
{
  uint16_t year;
  uint8_t hour = _hour0;
  uint8_t month, day;
  char buf[20];

  rtc_readdate(&year, &month, &day, NULL);

  // draw time
  GrContextFontSet(pContext, (tFont*)&g_sFontExNimbus91);

  sprintf(buf, "%02d", hour);
  GrStringDrawCentered(pContext, buf, 1, LCD_WIDTH / 4 + 10, 43, 0);
  GrStringDrawCentered(pContext, buf + 1, 1, LCD_WIDTH / 2 + LCD_WIDTH / 4 - 10, 43, 0);

  sprintf(buf, "%02d", _minute);
  GrStringDrawCentered(pContext, buf, 1, LCD_WIDTH / 4 + 10, 110, 0);
  GrStringDrawCentered(pContext, buf + 1, 1, LCD_WIDTH / 2 + LCD_WIDTH/4 - 10, 110, 0);

  GrContextFontSet(pContext, &g_sFontGothic18b);
  sprintf(buf, "%s %d, %d", toMonthName(month, 1), day, year);
  GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 157, 0);
}

static void drawClock10(tContext *pContext)
{	
  	uint16_t year;
  	uint8_t hour = _hour0;
  	uint8_t month, day;
  	uint8_t ispm = 0;
  	char buf[20];	

  	rtc_readdate(&year, &month, &day, NULL);
  	
  	// draw time
  	adjustAMPM(hour, &hour, &ispm);
  	
  	GrContextForegroundSet(pContext, ClrWhite); 	
  	sprintf(buf, "%02d:%2d", hour, _minute); 	
  	GrContextFontSet(pContext, &g_sFontMetroid50b);
  	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 50, 0);
  	
  	if (ispm) buf[0] = 'P';
    	else buf[0] = 'A';
  	buf[1] = 'M';
  	GrContextFontSet(pContext, &g_sFontMetroid20);
  	GrStringDraw(pContext, buf, 2, 104 , 77, 0);
  	
  	sprintf(buf, "%d.%02d.%02d", year, month, day); 	
  	GrContextFontSet(pContext, &g_sFontMetroid16b);
  	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 147, 0);
  		
}

static void drawClock11(tContext *pContext)
{	
  	uint16_t year;
  	uint8_t hour = _hour0;
  	uint8_t month, day;
  	uint8_t ispm = 0;
  	char buf[20];	
  	  	
  	rtc_readdate(&year, &month, &day, NULL);
  	
  	// draw time
  	adjustAMPM(hour, &hour, &ispm);
  	  	
 	//draw rectangle
 	GrContextForegroundSet(pContext, ClrWhite); 	
 	tRectangle rect = {31, 28, 112 , 118 };
 	GrRectFillRound(pContext, &rect, 8);
 	
 	GrContextForegroundSet(pContext, ClrBlack);
 	tRectangle rect1 = {32, 29, 111 , 117 };
 	GrRectFillRound(pContext, &rect1, 8);
 	
 	GrContextForegroundSet(pContext, ClrWhite); 	
 	sprintf(buf, "%d", day);
 	GrContextFontSet(pContext, &g_sFontRonda28);
 	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 56, 0);
 	
 	sprintf(buf, "%s", toMonthName(month, 0));
 	GrContextFontSet(pContext, &g_sFontHelvetic20);
 	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 91, 0);
 	
	sprintf(buf, "%02d:%02d", hour, _minute); 	
 	GrContextFontSet(pContext, &g_sFontRonda16);
 	GrStringDrawCentered(pContext, buf, -1, 54 , 146, 0);
 	
 	if (ispm) buf[0] = 'p';
    	else buf[0] = 'a';
  	buf[1] = 'm';	
  	
  	GrStringDrawCentered(pContext, buf, 2, 105 , 146, 0);
 	
  	
}

static void drawClock12(tContext *pContext)
{	
	uint8_t hour = _hour0;
  	uint8_t month, day;
  	uint8_t ispm = 0;
  	char buf[20];	
  	
  	GrContextForegroundSet(pContext, ClrWhite);
  	
  	  // draw time
  	adjustAMPM(hour, &hour, &ispm);
  	
  	sprintf(buf, "%02d", hour); 	
  	GrContextFontSet(pContext, &g_sFontMetroid60);
  	GrStringDraw(pContext, buf, 2, 21 , 35, 0);
  	
  	sprintf(buf, "%02d", _minute); 	
  	GrContextFontSet(pContext, &g_sFontMetroid84);
  	GrStringDraw(pContext, buf, 2, 21 , 92, 0);
  	
  	GrContextFontSet(pContext, &g_sFontMetroid40);
  	if (ispm) buf[0] = 'P';
    	else buf[0] = 'A';
    	GrStringDraw(pContext, buf, 1, 107 , 32, 0);	
    	buf[0] = 'M';	
    	GrStringDraw(pContext, buf, 1, 102 , 59, 0);	
  	
}

static void drawClock13(tContext *pContext)
{
	uint8_t ispm = 0;
	uint8_t hour = _hour0;
  	char buf[20];
  	char tokensep[] = " ";
  	char* buffer;
  	char *buf1, *buf2;
  	uint8_t Digi_h=0;
  	
  	// draw time
  	adjustAMPM(hour, &hour, &ispm);
  	
  	buffer = toEnglish(hour, buf);
  	GrContextFontSet(pContext, &g_sFontHelvetic32);
  	GrStringDrawCentered(pContext, buffer, -1,  LCD_WIDTH / 2, 54, 0);
			   	
  	buffer = toEnglish(_minute, buf);
	buf1 = strtok(buffer, tokensep);

	buf2 = strtok(NULL, tokensep);
	if(buf2!=NULL)
	{
		GrContextFontSet(pContext, &g_sFontHelvetic24);
		GrStringDrawCentered(pContext, buf1, -1,  LCD_WIDTH / 2, 94, 0);	
		
		GrContextFontSet(pContext, &g_sFontHelvetic20);
  		GrStringDrawCentered(pContext, buf2, -1, 45, 128, 0);	
	}	
	else
	{
		GrContextFontSet(pContext, &g_sFontHelvetic20);
		GrStringDrawCentered(pContext, buf1, -1,  45, 128, 0);	
	}	
	
	if (ispm) buf[0] = 'P';
	else buf[0] = 'A';
  	buf[1] = 'M';
  	GrStringDrawCentered(pContext, buf, 2, 120, 128, 0);
	
	
}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
static const draw_function Clock_selections[] =
{
  drawClock10,
  drawClock11,
  drawClock12,
  drawClock13,
  drawClock10,
  drawClock11,
  drawClock12,
  drawClock13,
  drawClock10,
  drawClock11,
};
#else
static const draw_function Clock_selections[] =
{
  drawClock0,
  drawClock1,
  drawClock2,
  drawClock3,
  drawClock4,
  drawClock5,
  drawClock6,
  drawClock7,
  drawClock8,
  drawClock9,
};
#endif
uint8_t digitclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  if (ev == EVENT_WINDOW_CREATED)
  {
    rtc_readtime(&_hour0, &_minute, NULL);
    if (rparam == NULL)
      _selection = window_readconfig()->digit_clock;
    else
      _selection = (uint8_t)rparam - 1;
    rtc_enablechange(MINUTE_CHANGE);

    return 0x80;
  }
  else if (ev == EVENT_WINDOW_ACTIVE)
  {
    rtc_readtime(&_hour0, &_minute, NULL);
  }
  else if (ev == EVENT_WINDOW_PAINT)
  {
    // clear the region
    tContext *pContext = (tContext*)rparam;
    GrContextForegroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	GrRectFill(pContext, &client_clip);
#else    
    	GrRectFill(pContext, &fullscreen_clip);
#endif    
    GrContextForegroundSet(pContext, ClrWhite);

    Clock_selections[_selection](pContext);
  }
  else if (ev == EVENT_TIME_CHANGED)
  {
    struct datetime* dt = (struct datetime*)rparam;
    _hour0 = dt->hour;
    _minute = dt->minute;
    window_invalid(NULL);
  }
  else if (ev == EVENT_KEY_PRESSED)
  {
    if (lparam == KEY_DOWN)
    {
      _selection += 0x1;
      if (_selection > sizeof(Clock_selections)/sizeof(draw_function) - 1)
      {
        _selection = 0x00;
      }
      window_invalid(NULL);
    }
    else if (lparam == KEY_UP)
    {
      _selection -= 0x1;
      if (_selection == 0xff)
      {
        _selection = sizeof(Clock_selections)/sizeof(draw_function) - 1;
      }
      window_invalid(NULL);
    }
  }
  else if (ev == EVENT_WINDOW_CLOSING)
  {
    rtc_enablechange(0);

    window_readconfig()->default_clock = 1;
    if (_selection != window_readconfig()->digit_clock)
    {
      window_readconfig()->digit_clock = _selection;
      window_writeconfig();
    }
  }
  else
  {
    return 0;
  }

  return 1;
}
