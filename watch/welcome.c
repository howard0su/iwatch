#include "contiki.h"
#include "grlib/grlib.h"
#include "window.h"

#include "Template_Driver.h"

extern const unsigned char logoPixel[];
static uint8_t index;

//static const char chinese[] = {0xE6, 0xAC, 0xA2, 0xE8, 0xBF, 0x8E, 0x00};
static const char *welcomes[] = {
	"Welcome",
	"Bienvenue",
	"Welkom",
	"欢迎",
};

#define SPAN 24

static void OnDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  static const tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
  GrRectFill(pContext, &rect);

  // draw the log
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  GrImageDraw(pContext, logoPixel, 8, 20);

  // draw welcome
  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
  //GrContextFontSet(pContext, (tFont*)&g_sFontGothic14);
  GrStringCodepageSet(pContext, CODEPAGE_UTF_8);
  for (int i = 0; i < 4; i++)
  	GrStringDrawCentered(pContext, welcomes[i], -1, LCD_X_SIZE - index + i * 20, 70 + i * 20, 0);

  GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);
}

uint8_t welcome_process(uint8_t ev, uint16_t lparam, void* rparam)
{
	static const tRectangle rect = {0, 60, LCD_X_SIZE, 160};
	switch(ev)
	{
		case EVENT_WINDOW_CREATED:
		case EVENT_WINDOW_ACTIVE:
			window_timer(CLOCK_SECOND/2);
			return 0x80;

		case EVENT_WINDOW_PAINT:
			OnDraw((tContext*)rparam);
			break;

		case PROCESS_EVENT_TIMER:
			index += 5;
			if (index > LCD_X_SIZE + LCD_X_SIZE/2)
				index = 0;
			window_invalid(&rect);
			window_timer(CLOCK_SECOND/2);
			break;

		case EVENT_KEY_PRESSED:
			if (lparam == KEY_DOWN)
				window_open(&btconfig_process, NULL);
			else if (lparam == KEY_UP)
				system_unlock();
			break;
	}

	return 1;

}