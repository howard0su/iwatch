#include "contiki.h"

#include "window.h"
#include <stdio.h>
#include "icons.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include "memory.h"

#include "stlv.h"
#include "stlv_client.h"
#include "ble_handler.h"

#include "cfs/cfs.h"
#include "btstack/include/btstack/utils.h"
#include "status.h"

char* location; 
uint8_t unit_type;	//1 be Fahrenheit and 0 be Celsius 
int     temperature;
int	max_temp;
int	min_temp;
uint8_t	weather_type;

static void onDraw(tContext *pContext)
{	
	char buf[30];
	
	GrContextForegroundSet(pContext, ClrWhite);
	tRectangle rect = {0, 0, 143 , 70 };
 	GrRectFill(pContext, &rect);
 	
 	GrContextBackgroundSet(pContext, ClrWhite);
 	GrContextForegroundSet(pContext, ClrBlack);
 	GrContextFontSet(pContext, (tFont*)&g_sFontExIcon64);
// 	GrStringDrawCentered(pContext,"w", 1, 36, 72, 0);
// 	GrStringDrawCentered(pContext,(char *)&weather_type, 1, 36, 72, 0);
	 GrStringDraw(pContext, (char *)&weather_type, 1, 40, 8, 0);
// 	GrStringDraw(pContext, "w", 1, 40, 8, 0);
 	
 	GrContextForegroundSet(pContext, ClrWhite);
 	sprintf(buf, "%d", temperature);
 	GrContextFontSet(pContext, &g_sFontRonda28); 	
 	GrStringDraw(pContext, buf, -1, 14 , 85, 0);
 	
 	GrContextFontSet(pContext, &g_sFontRonda15b); 	
 	sprintf(buf, "%d", max_temp); 	
 	GrStringDraw(pContext, buf, -1, 109 , 86, 0);
 	
 	sprintf(buf, "%d", min_temp); 	
 	GrStringDraw(pContext, buf, -1, 109 , 106, 0);
 	 	
 	GrContextFontSet(pContext, &g_sFontHelvetic20); 	
 	sprintf(buf, "%s", location); 	
 	GrStringDraw(pContext, buf, -1, 13 , 145, 0);
    	
}

uint8_t weather_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  	switch(ev)
  	{
  	case EVENT_WINDOW_CREATED:
		location = "Shanghai";		
		unit_type =0;
		temperature = 25;
		max_temp = 30;
		min_temp = 20;
		weather_type = 'w';
		    	
  		break;
  		
  	case EVENT_WINDOW_PAINT:
  	{	
  		tContext *pContext = (tContext*)rparam;
    		GrContextForegroundSet(pContext, ClrBlack);
    		GrRectFill(pContext, &fullscreen_clip);
    		onDraw(pContext);
	}    		
    		break;
    		
    	case EVENT_KEY_PRESSED:
    		if (lparam == KEY_EXIT)
  		{
    			window_close();
    			return 1;
  		}
    		break;	

  	default:
    		return 0;
  	}
  	return 1;	
}