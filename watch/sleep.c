#include "contiki.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "window.h"
#include "pedometer/sleepalgo.h"

#include <stdio.h>

#define LINEMARGIN 25
static void drawItem(tContext *pContext, uint8_t n, char icon, const char* text, const char* value)
{
  if (icon)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
    GrStringDraw(pContext, &icon, 1, 3, 30 + n * LINEMARGIN, 0);
  }

  // draw text
  GrContextFontSet(pContext, &g_sFontNova13);
  GrStringDraw(pContext, text, -1, 20, 30 + n * LINEMARGIN, 0);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_X_SIZE - width - 8, 30 + n * LINEMARGIN, 0);
}

static void formattime(char* buf, int minutes)
{
	uint16_t hours = minutes / 60;
	minutes = minutes % 60;

	sprintf(buf, "%d:%d", hours, minutes);
}

static uint8_t *ptr;
#include "pedometer/sleepalgo.h"
extern void mpu_switchmode(int d);
uint8_t test_sleep(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		ptr = (uint8_t*)malloc(128);
		if (ptr)
		{
			sleepalgo_init(ptr, 128);
			mpu_switchmode(2);
			window_timer(CLOCK_SECOND * 60);
		}
		break;

		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
      	  unsigned int available_minutes, lost_minutes;
      	  getslpdatainfo(&available_minutes, &lost_minutes);
          GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
		  char buf[20];
		  printf("samples: %d", available_minutes);
//		  hexdump(ptr, available_minutes/4);
		  
		  formattime(buf, getfallasleep_time());
		  drawItem(pContext, 0, 0, "Time to Sleep", buf);

		  formattime(buf, getwake_time());
		  drawItem(pContext, 1, 0, "Awake Time", buf);

		  formattime(buf, getsleeping_time());
		  drawItem(pContext, 2, 0, "Sleep", buf);

		  formattime(buf, getsleeping_time()/2); //XXX
		  drawItem(pContext, 3, 0, "Deep Sleep", buf);

		  sprintf(buf, "%d", 3);
		  drawItem(pContext, 4, 0, "Wake Times", buf);

		  // draw diagram
		  for(int i = 0; i < (available_minutes + 3)/4; i+=4)
		  {
			uint8_t data = ptr[i];
		  	for (int j = i; j < i + 4 && j < available_minutes; j++)
		  	{
		  		int length;
		  		switch(data & 0xC0)
		  		{
		  			case 0:
		  				length = 1;
		  				break;
		  			case 0x40:
		  				length = 6;
		  				break;
		  			case 0x80:
		  				length = 15;
		  				break;
		  			default:
		  				continue;
		  		}

		  		GrLineDrawV(pContext, j, LCD_Y_SIZE-length, LCD_Y_SIZE);
		  		data <<= 2;
		  	}
		  }

 		  break;
 		}

 		case EVENT_KEY_PRESSED:
 		{
 			window_invalid(NULL);
 			break;
 		}

 		case PROCESS_EVENT_TIMER:
 		{
 			window_invalid(NULL);
 			window_timer(CLOCK_SECOND * 60);
 			break;
 		}

 		case EVENT_WINDOW_CLOSING:
	 		printf("stop_slp_monitor()\n");
	 		mpu_switchmode(0);
	 		stop_slp_monitor();
	 		window_timer(0);
	 		if (ptr)
	 			free(ptr);
	 		ptr = NULL;
	 		break;

 		default:
 		return 0;
	}

	return 1;
}
