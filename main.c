#include <stdio.h>
#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

static tContext context;
void window_handle_event(uint8_t ev, void* data);

static void test_window(windowproc window, void* data)
{
	GrContextFontSet(&context, (const tFont*)NULL);
 	window(EVENT_WINDOW_CREATED, 0, data);
  	window(EVENT_WINDOW_PAINT, 0, &context);
  	GrFlush(&context);

  	window(EVENT_WINDOW_CLOSING, 0, 0);
}

static void* font;
static uint8_t testfont(uint8_t event, uint16_t lparam, void* rparam)
{
	switch(event)
	{
		case EVENT_WINDOW_CREATED:
		font = rparam;
		break;
		case EVENT_WINDOW_PAINT:
		{
			tContext* pContext = (tContext*)rparam;
		  GrContextForegroundSet(pContext, ClrBlack);
		  GrRectFill(pContext, &client_clip);

		  GrContextForegroundSet(pContext, ClrWhite);
	  	GrContextFontSet(pContext, (const tFont*)font);

		GrStringDraw(pContext, "01234567890", -1, 0, 17, 0);
		GrStringDraw(pContext, "abcdefghijk", -1, 0, 52, 0);
		GrStringDraw(pContext, "ABCDEFGHIJK", -1, 0, 92, 0);
		break;
		}
	}

	return 1;
}

static const tFont *fonts[] =
{
 &g_sFontNova12b,
 &g_sFontNova13,
 &g_sFontNova16,
 &g_sFontNova16b,
 &g_sFontNova38,
 &g_sFontNova38b,
 &g_sFontNova50b,
 //&g_sFontBaby16,
 //&g_sFontBaby12,
 //&g_sFontRed13,
 NULL
};

int main()
{
	memlcd_DriverInit();
	GrContextInit(&context, &g_memlcd_Driver);
	window_init();

	for(int i = 0; fonts[i]; i++)
		test_window(&testfont, (void*)fonts[i]);

    test_window(&worldclock_process, NULL);

    test_window(&sporttype_process, NULL);

    test_window(&calendar_process, NULL);

    test_window(&today_process, NULL);

    test_window(&countdown_process, NULL);
    
    // test menu in the last
	test_window(&menu_process, NULL);

	for (int i = 1; i <= 6; ++i)
	{
		test_window(&analogclock_process, (void*)(i + (i << 4)));
	}

	for (int i = 1; i <= 5; ++i)
	{
    	test_window(&digitclock_process, (void*)i);
	}

	window_notify("This is title", "this is message. blah", NOTIFY_OK, 0);
    window_close();

    printf("test finished!\n");
}