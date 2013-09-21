#include "contiki.h"
#include "CuTest.h"
#include <stdio.h>
#include <string.h>
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "sys/timer.h"
#include "sys/etimer.h"

#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

#include "watch/test.h"

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

struct _event
{
   int delta;
   uint8_t event;
   void* rparam;
   uint16_t lparam;
};

static struct _event test_events[] = {
    // today's activity
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

    // analog watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

    // digit watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   // world clock
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_UP, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   // calendar 
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   // stop watch
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_UP, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},

   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   // 
   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   {1, EVENT_KEY_PRESSED, (void*)KEY_ENTER, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_EXIT, 0},
   {1, EVENT_KEY_PRESSED, (void*)KEY_DOWN, 0},

   {-1}
};

PROCESS(event_process, "Test Event Driver");
static uint8_t run = 1;
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
       process_post(ui_process, test_events[event].event, test_events[event].rparam);
       event++;
       if (test_events[event].delta == -1)
       {
         run = 0;
       }
       etimer_set(&trigger, test_events[event].delta * CLOCK_SECOND / 100);
    }
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

void SimluateRun(CuTest* tc)
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
  while(run) {
    int r;
    do {
      /* Reset watchdog. */
      r = process_run();
    } while(r > 0);

#if 0
    int n = etimer_next_expiration_time();
    if (n > 0)
    {
      int p = n - clock_time();
      if (p > 0)
      nanosleep(p);
    }
    else
    {
      nanosleep(1000);
    }
#endif
    etimer_request_poll();
  }
}


static tContext context;
extern const tRectangle status_clip;
void window_handle_event(uint8_t ev, void* data);

  struct _event default_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {-1}
  };



static void run_window_events(windowproc window, struct _event *events)
{
  //GrContextFontSet(&context, (const tFont*)NULL);
  GrContextClipRegionSet(&context, &client_clip);

  for(struct _event *ev = events; ev->delta != -1; ev++)
  {
    window(ev->event, ev->lparam, ev->rparam);  
    if (ev->event == EVENT_WINDOW_PAINT)
    {
      GrFlush(&context);    
    }
  }
}

static void test_window(windowproc window, void* data)
{
 struct _event my_events[] = {
    {1, EVENT_WINDOW_CREATED, data, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {-1}
  };

  run_window_events(window, my_events);

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

void TestSportWatch(CuTest* tc)
{
  struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {1, EVENT_TIME_CHANGED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {-1}
  };

  for(int i = 0; i < 3; i++)
  {
    window_readconfig()->sports_grid = i;
    run_window_events(&sportswatch_process, test_events);

    int fd = cfs_open("/sports/000.bin", CFS_READ);
    CuAssert(tc, "file sports/000.bin doesn't find", fd != -1);
    uint16_t sig;
    cfs_read(fd, &sig, sizeof(sig));
    CuAssertIntEquals_Msg(tc, "sport file signautre doesn't match", 0x1517, sig);

    int offset = cfs_seek(fd, 0, CFS_SEEK_END);
    CuAssertIntEquals(tc, sizeof(uint16_t) +  // signature
                          sizeof(uint8_t) +   // grid num
                          (i + 3) * sizeof(uint8_t) +  // grid types
                          (i + 3 + 1) * sizeof(uint16_t) * 2, // data, 2 copy
                          offset);

    cfs_close(fd);
  }
}

void TestTestButton(CuTest* tc)
{
  struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_ENTER},
    {1, EVENT_KEY_PRESSED, NULL, KEY_EXIT},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {1, EVENT_KEY_LONGPRESSED, NULL, KEY_EXIT},
    {-1}
  };  
  run_window_events(&test_button, test_events);
}

void TestTestAnt(CuTest *tc)
{
  struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_ENTER},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_UP},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_DOWN},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {-1}
  };  

  run_window_events(&test_ant, test_events);
}

void TestTestLight(CuTest* tc)
{
  struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_UP},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_UP},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_DOWN},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {-1}
  };  
  run_window_events(&test_light, test_events);
}

void TestTestLcd(CuTest* tc)
{
  struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {1, EVENT_KEY_PRESSED, NULL, KEY_ENTER},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {-1}
  };  
  run_window_events(&test_lcd, test_events);
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

                  GrStringDraw(pContext, "01234567890", -1, 0, 15, 0);
                  GrStringDraw(pContext, "abcdefghijk", -1, 0, 35, 0);

                  GrStringCodepageSet(pContext, CODEPAGE_UTF_16);
                  //GrCodepageMapTableSet(pContext, GrMapUTF8_Unicode, 1);                  
                  GrStringDraw(pContext, L"中文测试", -1, 0, 55, 0);
                  GrStringDraw(pContext, L"中国语テスト", -1, 0, 75, 0);
                  GrStringDraw(pContext, L"중국어 테스트", -1, 0, 95, 0);
                  GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);                  

                  break;
                }
        }

        return 1;
}

void TestWideFont(CuTest* tc)
{
    struct _event _events[] = {
    {1, EVENT_WINDOW_CREATED, &g_sFontUnicode, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {-1}
  };

  FILE* fp = fopen("fontunicod16pt.bin", "rb");
  int fd = cfs_open("fontunicode", CFS_WRITE);

  if (fp != NULL && fd != -1)
  {
    // copy the file
    char buf[1024];
    int length;

    length = fread(buf, 1, 1024, fp);
    while(length > 0)
    {
      int wl = cfs_write(fd, buf, length);
      if (wl != length)
      {
        CuFail(tc, "Failed to write the font file into spi flash because falsh is too small");
      }
      length = fread(buf, 1, 1024, fp);
    }
    fclose(fp);
    cfs_close(fd);
  }

  CFSFontWrapperLoad("fontunicode");
  run_window_events(testfont, _events);
}


void TestWindows(CuTest *tc)
{ 
  //load_script("script1.amx", rom);
  //test_window(&script_process, rom);
  test_window(&script_process, "/script1.amx");
  test_window(&script_process, "/notexist.amx");

  for(int i = 0; fonts[i]; i++)
    test_window(&testfont, (void*)fonts[i]);
  test_window(&worldclock_process, NULL);

  test_window(&today_process, NULL);

  test_window(&sporttype_process, NULL);

  test_window(&calendar_process, NULL);

  test_window(&today_process, NULL);

  test_window(&countdown_process, NULL);

  // test menu in the last
  test_window(&menu_process, NULL);
  test_window(&menu_process, "Watch Setup");
  test_window(&menu_process, "About");

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
}

#include "btstack/src/hfp.h"
void hfp_test_setstatus(uint8_t ind, uint8_t value);
void TestPhoneScreen(CuTest* tc)
{
    struct _event test_events[] = {
    {1, EVENT_WINDOW_CREATED, NULL, 0},
    {2, EVENT_WINDOW_PAINT, &context, 0},
    {3, EVENT_WINDOW_CLOSING, NULL, 0},
    {-1}
  };

  hfp_test_setstatus(HFP_CIND_CALL, HFP_CIND_CALL_ACTIVE);
  run_window_events(phone_process, test_events);

  hfp_test_setstatus(HFP_CIND_CALL, HFP_CIND_CALL_NONE);
  hfp_test_setstatus(HFP_CIND_CALLSETUP, HFP_CIND_CALLSETUP_INCOMING);
  run_window_events(phone_process, test_events);  

  hfp_test_setstatus(HFP_CIND_CALL, HFP_CIND_CALL_NONE);
  hfp_test_setstatus(HFP_CIND_CALLSETUP, HFP_CIND_CALLSETUP_OUTGOING);
  run_window_events(phone_process, test_events);   

  // test siri
  test_events[0].rparam = 1;
  hfp_test_setstatus(HFP_CIND_CALL, HFP_CIND_CALL_ACTIVE);
  run_window_events(phone_process, test_events);
}

CuSuite* WindowGetSuite(void)
{
	CuSuite* suite = CuSuiteNew("Window Test");
 
  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  window_init();
  status_process(EVENT_WINDOW_CREATED, 0, NULL);

  SUITE_ADD_TEST(suite, TestWideFont);

  SUITE_ADD_TEST(suite, TestSportWatch);
  SUITE_ADD_TEST(suite, TestTestButton);
  SUITE_ADD_TEST(suite, TestTestLight);
  SUITE_ADD_TEST(suite, TestTestLcd);
  SUITE_ADD_TEST(suite, TestTestAnt);
  SUITE_ADD_TEST(suite, TestPhoneScreen);


  SUITE_ADD_TEST(suite, TestWindows);
  SUITE_ADD_TEST(suite, SimluateRun);

  return suite;
}
