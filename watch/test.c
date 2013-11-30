#include "contiki.h"
#include "window.h"
#include "test.h"
#include "system.h"
#include "Template_Driver.h"
#include "backlight.h"
#include "ant/ant.h"
#include "ant/antinterface.h"
#include "btstack/src/hci.h"

#include <stdio.h>
#include <stdlib.h>

static uint8_t data;
static uint8_t onoff;
static const uint8_t str[] = {
	45, 78, 135, 101, 75, 109, 213, 139, 198, 48, 185, 48, 200, 48, 0, 0
};
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

#if 0
 		  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
	      GrStringCodepageSet(pContext, CODEPAGE_UTF_16);
	      //GrCodepageMapTableSet(pContext, GrMapUTF8_Unicode, 1);                  
	      GrStringDraw(pContext, (char*)str, -1, 2, 40, 0);
	      GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);    
#endif
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

uint8_t test_mpu6050(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		data = mpu6050_selftest();
		break;

		case EVENT_KEY_PRESSED:
		{
			if (lparam == KEY_ENTER)
			{
				data = mpu6050_selftest();
  			window_invalid(NULL);
			}

			break;
		}
		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);
		  GrContextForegroundSet(pContext, ClrWhite);

      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
		  if (data == 0)
		  {
				GrStringDraw(pContext, "MPU6050 passed self testing", -1, 32, 16, 0);
		  }
		  else
		  {
		  	GrStringDraw(pContext, "MPUT6050 failed self testing", -1, 32, 16, 0);
		  }

		  GrStringDraw(pContext, "watch face up", -1, 32, 34, 0);
			window_button(pContext, KEY_ENTER, "Retest");

 		  break;
 		}

 		default:
	 		return 0;
	}

	return 1;
}


static const uint8_t HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd[] = 
{
    //HCI_VS_DRPb_Tester_Packet_TX_RX
    0x85, 0xFD, 12, 0x03, 0, 0xFF, 0x01, 0x02, 0x00, 0x1b, 0x00, 15, 0x01, 0xFF, 0x01
};

uint8_t test_bluetooth(uint8_t ev, uint16_t lparam, void* rparam)
{
	uint8_t buf[sizeof(HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd)];
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		onoff = 0;
		data = 0;
		break;

		case EVENT_KEY_PRESSED:
		{
			if (lparam == KEY_ENTER)
			{
				onoff = 1 - onoff;
			}

			if (lparam == KEY_UP && data <= 78)
			{
				data++;
			}

			if (lparam == KEY_DOWN && data > 0)
			{
				data--;
			}

			// copy

			if (onoff)
			{
				memcpy(buf, HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd, sizeof(HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd));
				buf[4] = data;
				hci_send_cmd_packet(buf, sizeof(HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd));
			}
			else
			{
				buf[0] = 0x88;
				buf[1] = 0xFD;
				buf[2] = 0;
				hci_send_cmd_packet(buf, 3);
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
 		  	GrStringDraw(pContext, "BT RX/TX is on", -1, 32, 16, 0);
 		  else
 		  	GrStringDraw(pContext, "BT RX/TX is off", -1, 32, 16, 0);

 		  char buf[32];
		  sprintf(buf, "Freqency: %dMhz", data < 40 ? 
		  									2402 + data * 2:
		  									2403 + (data - 40) * 2);
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
		{
			uint8_t buf[3];
			buf[0] = 0x88;
			buf[1] = 0xFD;
			buf[2] = 0;
			hci_send_cmd_packet(buf, 3);

 		}
 		break;

 		default:
 		return 0;
	}

	return 1;
}


uint8_t test_dut(uint8_t ev, uint16_t lparam, void* rparam)
{
	uint8_t buf[sizeof(HCI_VS_DRPb_Tester_Packet_TX_RX_Cmd)];
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
			bluetooth_enableDUTMode();
		break;

		case EVENT_WINDOW_PAINT:
		{
		  tContext *pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
      GrContextFontSet(pContext, (tFont*)&g_sFontBaby16);
      
	  	GrStringDraw(pContext, "BT DUT mode is on", -1, 32, 16, 0);
 		  break;
 		}

 		default:
 		return 0;
	}

	return 1;
}
