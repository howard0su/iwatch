#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <unistd.h>
#include "sys/timer.h"
#include "sys/etimer.h"

static tContext context;
extern const tRectangle status_clip;
void window_handle_event(uint8_t ev, void* data);

static void test_window(windowproc window, void* data)
{
  GrContextFontSet(&context, (const tFont*)NULL);
  window(EVENT_WINDOW_CREATED, 0, data);
  GrContextClipRegionSet(&context, &status_clip);
  status_process(EVENT_WINDOW_PAINT, 0, &context);
  GrContextClipRegionSet(&context, &client_clip);
  GrContextForegroundSet(&context, ClrWhite);
  window(EVENT_WINDOW_PAINT, 0, &context);
  GrFlush(&context);

  window(EVENT_WINDOW_CLOSING, 0, 0);
}

static void test_window_stopwatch(windowproc window, void* data)
{
  GrContextFontSet(&context, (const tFont*)NULL);
  window(EVENT_WINDOW_CREATED, 0, data);
  GrContextClipRegionSet(&context, &status_clip);
  status_process(EVENT_WINDOW_PAINT, 0, &context);
  for(int i = 3; i >= 0; i--)
    window(EVENT_KEY_PRESSED, KEY_ENTER, (void*)0);
  GrContextClipRegionSet(&context, &client_clip);
  GrContextForegroundSet(&context, ClrWhite);
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

static const ui_config ui_config_default =
{
  UI_CONFIG_SIGNATURE,

  "Shanghai", "London", "New York", "Place A", "Place B", "Place C",
  +16, +8, +3, -1, -2, -3,

  1,
  4,
  2,

  1,
  0, 1, 2, 3, 4
};

extern ui_config ui_config_data;

static struct 
{
   int delta;
   uint8_t event;
   void* data
} test_events[] = {
    // today's activity
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

    // analog watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

    // digit watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   // world clock
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_UP},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   // calendar 
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   // stop watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_UP},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},

   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   // 
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN},

   {-1}
};

PROCESS(event_process, "Test Event Driver");

PROCESS_THREAD(event_process, ev, data)
{
  static int event = 0;
  static struct etimer trigger;
  PROCESS_BEGIN();
  etimer_set(&trigger, test_events[event].delta * CLOCK_SECOND/10);
  while(1)
  {
    if (ev == PROCESS_EVENT_TIMER)
    {
       process_post(ui_process, test_events[event].event, test_events[event].data);
       event++;
       if (test_events[event].delta == -1)
       {
         exit(0);
       }
       etimer_set(&trigger, test_events[event].delta * CLOCK_SECOND / 100);
    }
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}


void SimluateRun()
{

 /*
  * Initialize Contiki and our processes.
  */
  process_init();
  process_start(&etimer_process, NULL);
  ctimer_init();

  energest_init();
  ENERGEST_ON(ENERGEST_TYPE_CPU);

  window_init();

  autostart_start(autostart_processes);

  process_start(&event_process, NULL);
  while(1) {
    int r;
    do {
      /* Reset watchdog. */
      r = process_run();
    } while(r > 0);

    int n = etimer_next_expiration_time();
    if (n > 0)
    {
      int p = n - clock_time();
      printf("expre in %d\n", p);
      if (p > 0)
      nanosleep(p);
    }
    else
    {
      nanosleep(1000);
    }
    etimer_request_poll();
  }
}

int main()
{
  memcpy(&ui_config_data, &ui_config_default, sizeof(ui_config));

#if 1
  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  window_init();
  status_process(EVENT_WINDOW_CREATED, 0, NULL);

/*
  for(int i = 0; fonts[i]; i++)
    test_window(&testfont, (void*)fonts[i]);
  */
  test_window(&worldclock_process, NULL);

  test_window(&today_process, NULL);

  test_window(&sporttype_process, NULL);

  for(int i = 0; i < 3; i++)
  {
    window_readconfig()->sports_grid = i;
    test_window(&sportswatch_process, NULL);
  }

  test_window(&calendar_process, NULL);

  test_window(&today_process, NULL);

  test_window(&countdown_process, NULL);

  // test menu in the last
  test_window(&menu_process, NULL);

  test_window_stopwatch(&stopwatch_process, NULL);

  //test_window(&menu_process, 1);

  for (int i = 1; i <= 6; ++i)
    {
      test_window(&analogclock_process, (void*)i);
    }

  for (int i = 1; i <= 9; ++i)
    {
      test_window(&digitclock_process, (void*)i);
    }

  window_notify("Facebook", "From: Tom Paker\nOur schedule is crazy next a few days unfortunately.", NOTIFY_OK, 'a');
  //window_close();

  printf("test finished!\n");
#endif
  SimluateRun();
}
