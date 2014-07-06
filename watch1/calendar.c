#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include "rtc.h"
#include <stdio.h>

uint8_t month, now_month, day, now_day;
uint16_t year, now_year;
static const uint16_t lx_weekday[8] = {0, 20, 40, 60 , 83, 103, 123, 144};
static void OnDraw(tContext *pContext)
{
  	char buf[20];
  	// clear screen
  	GrContextForegroundSet(pContext, ClrBlack);
  	GrContextBackgroundSet(pContext, ClrWhite);
  	GrRectFill(pContext, &fullscreen_clip);


  	// draw table title
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrContextBackgroundSet(pContext, ClrBlack);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
	const tRectangle rect = {0, 42, 255, 57};
#else
  	const tRectangle rect = {0, 27, 255, 41};
#endif  
  	GrRectFill(pContext, &rect);

  	// draw the title bar
  
  	sprintf(buf, "%s %d", month_name[month - 1], year);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)  
	GrContextFontSet(pContext, &g_sFontBaby16b);
	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 30, 0);
#else  
	GrContextFontSet(pContext, &g_sFontGothic18b);
  	GrStringDrawCentered(pContext, buf, -1, LCD_WIDTH / 2, 15, 0);
#endif
  	
  	GrContextForegroundSet(pContext, ClrBlack);
  	GrContextBackgroundSet(pContext, ClrWhite);
  	
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
  	GrContextFontSet(pContext, &g_sFontBaby16);
#else
	GrContextFontSet(pContext, &g_sFontGothic18);
#endif    	
  	for(int i = 0; i < 7; i++)
  	{
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)      	
		GrStringDrawCentered( pContext, week_shortname[i], -1, lx_weekday[i] + 11, 50, 0);
		// draw line in title bar
		GrContextForegroundSet(pContext, ClrBlack);
//		GrLineDrawV(pContext, lx_weekday[i], 42, 57);
#else
    		GrStringDrawCentered( pContext, week_shortname[i], -1, i * 20 + 11, 35, 0);
    		// draw line in title bar
    		GrLineDrawV(pContext, i * 20, 28, 42);
#endif    
    		
    
  	}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
  	GrContextFontSet(pContext, &g_sFontBaby16);
#else
  	GrContextFontSet(pContext, &g_sFontGothic18);
#endif  
  	GrContextForegroundSet(pContext, ClrWhite);
  	GrContextBackgroundSet(pContext, ClrBlack);

  	// get the start point of this month
  	uint8_t weekday = rtc_getweekday(year, month, 1) - 1; // use 0 as index
  	uint8_t maxday = rtc_getmaxday(year, month);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	uint8_t y = 65;
#else  
  	uint8_t y = 50;
#endif
  	for(int day = 1; day <= maxday; day++)
  	{
    		sprintf(buf, "%d", day);

    		uint8_t today = now_year == year && now_month == month && now_day == day;
    		if (today)
    		{
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)       			
			const tRectangle rect = {lx_weekday[weekday] + 1, y - 7, lx_weekday[weekday+1] - 1, y + 7};
#else    			
      			const tRectangle rect = {weekday * 20 + 1, y - 7, 20 + weekday * 20 - 1, y + 7};
#endif      			
      			GrRectFill(pContext, &rect);
      			GrContextForegroundSet(pContext, ClrBlack);
      			GrContextBackgroundSet(pContext, ClrWhite);
    		}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
		else if( (weekday == 0)||(weekday == 6) )
		{
			const tRectangle rect = {lx_weekday[weekday], y - 7, lx_weekday[weekday+1] , y + 7};
      			GrRectFill(pContext, &rect);
      			GrContextForegroundSet(pContext, ClrBlack);
      			GrContextBackgroundSet(pContext, ClrWhite);			
		}	
		GrStringDrawCentered( pContext, buf, -1, lx_weekday[weekday] + 11, y, 0);
#else    
    		GrStringDrawCentered( pContext, buf, -1, weekday * 20 + 11, y, 0);
#endif    
    		if (today)
    		{
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)        			
			const tRectangle rect2 = {lx_weekday[weekday] + 16, y - 5, lx_weekday[weekday] + 17, y - 4};
#else
      			const tRectangle rect2 = {weekday * 20 + 16, y - 5, weekday * 20 + 17, y - 4};
#endif      			
      			GrRectFill(pContext, &rect2);

      			GrContextForegroundSet(pContext, ClrWhite);
      			GrContextBackgroundSet(pContext, ClrBlack);
    		}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
		else if( (weekday == 0)||(weekday == 6) )
		{
      			GrContextForegroundSet(pContext, ClrWhite);
      			GrContextBackgroundSet(pContext, ClrBlack);			
		}
#endif		
    		if (weekday != 6)
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
			GrLineDrawV(pContext, lx_weekday[weekday+1] , 58, y + 7);
#else    	
      			GrLineDrawV(pContext, (weekday + 1 ) * 20, 42, y + 7);
#endif
    		weekday++;
    		if (weekday == 7)
    		{
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
			GrLineDrawH(pContext, lx_weekday[1]+1, lx_weekday[6]-1 , y + 8);
#else    			
      			GrLineDrawH(pContext, 0, LCD_WIDTH, y + 8);
#endif	
      			weekday = 0;
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
			y += 16;
#else      
      			y += 20;
#endif      
    		}
  	}
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	GrLineDrawH(pContext, 0, lx_weekday[weekday] , y + 8);
#else  	
  	GrLineDrawH(pContext, 0, weekday * 20, y + 8);
#endif  	

  	// draw the buttons
	if (month == 1)
    		sprintf(buf, "%s %d", month_shortname[11], year - 1);
  	else
    		sprintf(buf, "%s %d", month_shortname[month - 2], year);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	window_button(pContext, KEY_UP|0x80, buf);
#else    
  	window_button(pContext, KEY_ENTER, buf);
#endif
  	if (month == 12)
    		sprintf(buf, "%s %d", month_shortname[0], year + 1);
  	else
    		sprintf(buf, "%s %d", month_shortname[month], year);
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
	window_button(pContext, KEY_DOWN|0x80, buf);
#else    
  	window_button(pContext, KEY_DOWN, buf);
#endif  
}


uint8_t calendar_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  	if (ev == EVENT_WINDOW_CREATED)
  	{
    		rtc_readdate(&year, &month, &day, NULL);
    		now_month = month;
    		now_year = year;
    		now_day = day;
    		return 0x80;
  	}
  	else if (ev == EVENT_KEY_PRESSED)
  	{
#if defined(PRODUCT_W002) || defined(PRODUCT_W004)    
		if (lparam == KEY_ENTER)
#else  	
    		if (lparam == KEY_ENTER)
#endif    	
    		{
      			if (month == 1)
      			{
        			month = 12;
        			year--;
      			}
      			else
      			{
        			month--;
      			}
      				window_invalid(NULL);
    		}
    		else if (lparam == KEY_DOWN)
    		{
      			if (month == 12)
      			{
        			month = 1;
        			year++;
      			}
      			else
      			{
        			month++;
      			}
      			window_invalid(NULL);
    		}
  	}
  	else if (ev == EVENT_WINDOW_PAINT)
  	{
    		OnDraw((tContext*)rparam);
  	}
  	else
  	{
    		return 0;
  	}

  	return 1;
}
