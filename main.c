#include <stdio.h>
#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

static tContext context;
void window_handle_event(uint8_t ev, void* data);

int main()
{
	memlcd_DriverInit();
	GrContextInit(&context, &g_memlcd_Driver);
	window_init();

    window_open(&worldclock_process, NULL);
	window_close();

    window_open(&sporttype_process, NULL);
	window_close();

    window_open(&calendar_process, NULL);
	window_close();

    window_open(&digitclock_process, NULL);
	window_close();

    window_open(&today_process, NULL);
	window_close();
    
    // test menu in the last
	window_open(&menu_process, NULL);
	window_handle_event(EVENT_KEY_PRESSED, (void*)KEY_DOWN);
	window_close();

	window_notify("This is title", "this is message. blah", NOTIFY_OK, 0);
    window_close();
}