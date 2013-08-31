#include "contiki.h"
#include "window.h"
#include "test.h"
#include "system.h"
#include "Template_Driver.h"
#include <stdio.h>
#include "backlight.h"

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
 		  for(int i = 0; i < 4; i++)
 		  {
 		  	char buf[30];
 		  	if (data & (1 << i))
 		  	{
 		  		sprintf(buf, "Key %d is ok", i);
 		  		GrStringDraw(pContext, buf, -1, 5, 40 + i * 16, 0);
 		  	}
 		  }
 		  GrContextFontSet(pContext, (tFont*)&g_sFontBaby12);
 		  GrStringDraw(pContext, "Long press exit to quit", -1, 2, 110, 0);
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
 		  break;
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