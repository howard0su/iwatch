#include <stdio.h>
#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

static tContext context;
void window_handle_event(uint8_t ev, void* data);

static void test_window(windowproc window, void* data)
{
 	window(EVENT_WINDOW_CREATED, 0, data);
  	window(EVENT_WINDOW_PAINT, 0, &context);
  	GrFlush(&context);

  	window(EVENT_WINDOW_CLOSING, 0, 0);
}

int main()
{
	memlcd_DriverInit();
	GrContextInit(&context, &g_memlcd_Driver);
	window_init();

    test_window(&worldclock_process, NULL);

    test_window(&sporttype_process, NULL);

    test_window(&calendar_process, NULL);

    test_window(&today_process, NULL);
    
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
}