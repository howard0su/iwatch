#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"

static void OnDraw(tContext* pContext)
{
	// clear the screen

	// draw the countdown time

	// draw the stoped times

}

uint8_t stopwatch_process(uint8_t event, uint16_t lparam, void* rparam)
{
	switch(event)
	{
		case EVENT_WINDOW_CREATED:
			break;
		case EVENT_WINDOW_PAINT:
			OnDraw((tContext*)rparam);
			break;

		default:
			return 0;
	}

	return 1;
}

