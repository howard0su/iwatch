#include "contiki.h"
#include "window.h"
#include "test.h"
#include "system.h"
#include "Template_Driver.h"
#include <stdio.h>
#include "backlight.h"
#include "ant/ant.h"
#include "ant/antinterface.h"

static uint8_t data;

uint8_t test_button(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		data = 0;
		break;

		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
  	      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
 		  GrStringDraw(pContext, "Test Buttons", -1, 32, 16, 0);

 		  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
	      GrStringCodepageSet(pContext, CODEPAGE_UTF_16);
	      //GrCodepageMapTableSet(pContext, GrMapUTF8_Unicode, 1);                  
	      GrStringDraw(pContext, (char*)L"中文测试", -1, 2, 40, 0);
	      GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);    

 		  for(int i = 0; i < 4; i++)
 		  {
 		  	char buf[30];
 		  	if (data & (1 << i))
 		  	{
 		  		sprintf(buf, "Key %d is ok", i);
 		  		GrStringDraw(pContext, buf, -1, 5, 60 + i * 16, 0);
 		  	}
 		  }
 		  GrContextFontSet(pContext, (tFont*)&g_sFontBaby12);
 		  GrStringDraw(pContext, "Long press exit to quit", -1, 2, 120, 0);
		  break;
		}
		case EVENT_KEY_PRESSED:
		{
			data |= 1 << lparam;
			window_invalid(NULL);
			break;
		}
		case EVENT_EXIT_PRESSED:
		{
			data |= 1 << KEY_EXIT;
			window_invalid(NULL);
			break;	
		}
		case EVENT_KEY_LONGPRESSED:
		{
			if (lparam == KEY_EXIT)
				window_close();
			break;
		}
		default:
		return 0;
	}

	return 1;
}

uint8_t test_motor(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		data = 0;
		break;

		case EVENT_KEY_PRESSED:
		{
			switch(lparam)
			{
				case KEY_UP:
				data++;
				if (data > 16) data = 1;
				break;
				case KEY_DOWN:
				data--;
				if (data == 0) data = 16;
				break;
				case KEY_ENTER:
				data = 0;
				break;
			}
			motor_on(data, 0);
			window_invalid(NULL);
			break;
		}
		case EVENT_WINDOW_PAINT:
		{
		  char buf[32];
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
  	      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
 		  GrStringDraw(pContext, "Test Motor", -1, 32, 16, 0);

    	  sprintf(buf, "Motor Level: %d", data);
 		  GrStringDraw(pContext, buf, -1, 5, 40, 0);

 		  window_button(pContext, KEY_UP, "+");
 		  window_button(pContext, KEY_DOWN, "-");
 		  window_button(pContext, KEY_ENTER, "Reset");

 		  break;
 		}
		case EVENT_EXIT_PRESSED:
		motor_on(0, 0);
		return 0; // return 0 to close the window
		default:
		return 0;
	}

	return 1;
}

uint8_t test_light(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		data = 0;
		break;

		case EVENT_KEY_PRESSED:
		{
			switch(lparam)
			{
				case KEY_UP:
				data++;
				if (data > 16) data = 1;
				break;
				case KEY_DOWN:
				data--;
				if (data == 0) data = 16;
				break;
				case KEY_ENTER:
				data = 0;
				break;
			}
			backlight_on(data);
			window_invalid(NULL);
			break;
		}
		case EVENT_WINDOW_PAINT:
		{
		  char buf[32];
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
  	      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
 		  GrStringDraw(pContext, "Test Lights", -1, 32, 16, 0);

    	  sprintf(buf, "Light Level: %d", data);
 		  GrStringDraw(pContext, buf, -1, 5, 40, 0);
 		  window_button(pContext, KEY_UP, "+");
 		  window_button(pContext, KEY_DOWN, "-");
 		  window_button(pContext, KEY_ENTER, "Reset");
 		  break;
 		}
		case EVENT_EXIT_PRESSED:
		backlight_on(0);
		return 0; // return 0 to close the window
		default:
		return 0;
	}

	return 1;
}

uint8_t test_lcd(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		data = 0;
		break;

		case EVENT_KEY_PRESSED:
		{
			if (lparam == KEY_UP)
				window_timer(CLOCK_SECOND);
			else if (lparam == KEY_DOWN)
				window_timer(0);

			window_invalid(NULL);
			break;
		}
		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  for(int x = 0; x < LCD_X_SIZE; x+=8)
		  	for(int y = 0; y < LCD_Y_SIZE; y+=8)
		  	{
		  		tRectangle rect = {
		  			x, y, x+8, y+8
		  		};
		  	if ((((x/8) & 1) ^ ((y/8) & 1)) == data)
		  		GrContextForegroundSet(pContext, ClrBlack);
		  	else
		  		GrContextForegroundSet(pContext, ClrWhite);

		  		GrRectFill(pContext, &rect);
			}
		  data = data ^ 1;
		  window_button(pContext, KEY_UP, "START");
		  window_button(pContext, KEY_DOWN, "STOP");
 		  break;
 		}
 		case PROCESS_EVENT_TIMER:
 		{
 			window_invalid(NULL);
 			window_timer(CLOCK_SECOND);
 		}
 		default:
 		return 0;
	}

	return 1;
}

uint8_t test_reboot(uint8_t ev, uint16_t lparam, void* rparam)
{
	system_rebootToNormal();
}

static uint8_t onoff;
uint8_t test_ant(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		onoff = 0;
		break;

		case EVENT_KEY_PRESSED:
		{
			if (lparam == KEY_ENTER)
			{
				if (onoff)
					ant_shutdown();
				else
					ant_init(MODE_HRM);
				onoff ^= 1;
			}

			if (lparam == KEY_UP)
			{
				data++;
				if (data > 4) data = 4;
				if (onoff)
					ANT_ChannelPower(0, data);
			}

			if (lparam == KEY_DOWN)
			{
				if (data > 0)
					data--;
				if (onoff)
					ANT_ChannelPower(0, data);
			}

			window_invalid(NULL);
			break;
		}
		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
  	      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
  	      if (onoff)
 		  	GrStringDraw(pContext, "ANT is on", -1, 32, 16, 0);
 		  else
 		  	GrStringDraw(pContext, "ANT is off", -1, 32, 16, 0);

 		  char buf[32];
		  sprintf(buf, "Tx Power Level: %d", data);
 		  GrStringDraw(pContext, buf, -1, 5, 40, 0);

 		  window_button(pContext, KEY_UP, "+");
 		  window_button(pContext, KEY_DOWN, "-");
 		  if (onoff)
 		  	window_button(pContext, KEY_ENTER, "Off");
 		  else
 		  	window_button(pContext, KEY_ENTER, "On");
 		  break;
 		}
 		case EVENT_WINDOW_CLOSING:
 		if (onoff)
 			ant_shutdown();
 		break;

 		default:
 		return 0;
	}

	return 1;
}