#include "contiki.h"

#include "window.h"
#include "rtc.h"
#include <stdio.h>
#include "memory.h"
#include "memlcd.h"

#define index d.world.index

extern void adjustAMPM(uint8_t hour, uint8_t *outhour, uint8_t *ispm);

static void drawItem(tContext *pContext,
                     uint8_t y,
                     const char* name,
                     uint8_t hour, uint8_t minute,
                     uint8_t month, uint8_t day)
{
  char buf[20];
  uint8_t ispm;
  adjustAMPM(hour, &hour, &ispm);


  GrContextForegroundSet(pContext, ClrWhite);
  
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)	
	GrContextForegroundSet(pContext, ClrWhite);
	tRectangle rect = {0 , 17 + y , LCD_WIDTH , 51 + y };
	GrRectFillRound(pContext, &rect, 0);
	
	GrContextForegroundSet(pContext, ClrBlack);
	GrContextFontSet(pContext, (tFont*)&g_sFontHelvetic18);
	GrStringDraw(pContext, name, -1, 32, y + 27, 0);
	
	tRectangle rect1 = {0 , 52 + y , LCD_WIDTH , 91 + y };
	GrRectFillRound(pContext, &rect1, 0);
	
	GrContextForegroundSet(pContext, ClrWhite);
	GrContextFontSet(pContext, (tFont*)&g_sFontRonda18);
	//GrContextFontSet(pContext, (tFont*)&g_sFontRonda15b);
	sprintf(buf, "%02d:%02d", hour, minute);
	GrStringDraw(pContext, buf, -1, 32, y + 55, 0);
	
	GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
	sprintf(buf, "%s", ispm?"pm":"am");
	GrStringDraw(pContext, buf, -1, 92, y + 64, 0);	
	
	
#else  
  if (y)
    GrLineDrawH(pContext, 0, 144,  y);

  GrContextFontSet(pContext, (tFont*)&g_sFontGothic18);
  GrStringDraw(pContext, name, -1, 12, y + 10, 0);

  GrContextFontSet(pContext, (tFont*)&g_sFontGothic18);
  sprintf(buf, "%02d:%02d %s", hour, minute, ispm?"PM":"AM");
  GrStringDraw(pContext, buf, -1, 12, y + 30, 0);

  char str[8];
  sprintf(str, "%d/%d", month, day);

  int height = GrStringHeightGet(pContext);
  int width = GrStringWidthGet(pContext, str, -1);
  GrContextForegroundSet(pContext, ClrWhite);
  tRectangle rect = {86, y + 30, 86 + width + 2, y + 30 + height};
  GrRectFillRound(pContext, &rect, 2);

  GrContextForegroundSet(pContext, ClrBlack);
  GrStringDraw(pContext, str, -1, 88, y + 30, 0);
#endif  
}

static void onDraw(tContext *pContext)
{
	uint8_t month, day;
  	// clear the screen
  	GrContextForegroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
	GrRectFill(pContext, &client_clip);
#else  	
  	GrRectFill(pContext, &fullscreen_clip);
#endif  	
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrContextBackgroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
  	for(int i = 0; i < 2; i++)	
#else
  	for(int i = 0; i < 3; i++)
#endif  
  	{
    		int8_t hour;
    		uint8_t minute;
    		rtc_readtime((uint8_t*)&hour, &minute, NULL);
    		rtc_readdate(NULL, &month, &day, NULL);

    		int8_t offset = window_readconfig()->worldclock_offset[i + index];
    		hour += offset;
    		if (hour > 24)
    		{
      			day++;
      			hour -= 24;
    		}
    		else if (hour < 0)
    		{
      			day--;
      			hour += 24;
    		}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
    		drawItem(pContext, i * 75,  window_readconfig()->worldclock_name[i + index],
             	hour, minute, month, day);    		
    		for(int i = 0; i < 6; i++)
    		{
      			
      			if(index==0)
      			{
      				GrContextForegroundSet(pContext, ClrWhite);
        			GrLineDrawH(pContext, 130 - i, 130 + i,  160 - i);      				
      			}	
      			else if (index==4)
      			{	
      				GrContextForegroundSet(pContext, ClrBlack);
        			GrLineDrawH(pContext, 130 - i, 130 + i,  15 + i);
      			}
      			else
      			{	
      				GrContextForegroundSet(pContext, ClrBlack);
        			GrLineDrawH(pContext, 130 - i, 130 + i,  15 + i);      				
      				GrContextForegroundSet(pContext, ClrWhite);
        			GrLineDrawH(pContext, 130 - i, 130 + i,  160 - i);
        		}	
    		}             	
#else
    		drawItem(pContext, i * 55,  window_readconfig()->worldclock_name[i + index],
             	hour, minute, month, day);
    		GrContextForegroundSet(pContext, ClrWhite);
    		for(int i = 0; i < 6; i++)
    		{
      			if (index)
        			GrLineDrawH(pContext, 130 - i, 130 + i,  15 + i);
      			else
        			GrLineDrawH(pContext, 130 - i, 130 + i,  160 - i);
    		}             	
#endif

  	}
}

uint8_t worldclock_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    index = 0;
    return 0x80;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_KEY_PRESSED:
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
	if (lparam == KEY_UP && index == 2)
	{
		index = 0;
      		window_invalid(NULL);
	}	
	else if ( (lparam == KEY_UP && index == 4) || (lparam == KEY_DOWN && index == 0) )
	{
		index = 2;
      		window_invalid(NULL);		
	}	
	else if (lparam == KEY_DOWN && index == 2)
	{
		index = 4;
      		window_invalid(NULL);				
	}
#else  	
    if (lparam == KEY_UP && index == 3)
    {
      index = 0;
      window_invalid(NULL);
    }
    else if (lparam == KEY_DOWN && index == 0)
    {
      index = 3;
      window_invalid(NULL);
    }
#endif    
    break;
  default:
    return 0;
  }

  return 1;
}
