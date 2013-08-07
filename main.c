#include <stdio.h>
#include <string.h>
#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <unistd.h>
#include "sys/timer.h"
#include "sys/etimer.h"

#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"

static tContext context;
extern const tRectangle status_clip;
void window_handle_event(uint8_t ev, void* data);

int16_t inputpoints[] = {
  -2,0,19, -2,0,19, -2,0,19, -2,0,19, -2,0,19, -2,0,19, -2,0,19, -3,0,20, -3,0,20, -3,0,20, -3,0,20, -3,0,20, -3,0,20, -3,
0,20, -3,0,20, -3,0,20, -4,0,20, -4,0,20, -3,0,20, -3,0,20, -2,1,19, -1,2,18, -1,2,17, 0,2,18, 0,2,17, 0,2,17, 0,3,16, 
1,3,16, 3,3,16, 3,3,16, 4,3,16, 4,3,16, 4,3,16, 4,3,17, 3,3,16, 3,3,16, 4,3,16, 4,3,16, 5,3,16, 5,2,16, 5,1,16, 5,0,16, 
6,1,17, 6,1,19, 7,2,20, 9,3,20, 10,4,20, 10,5,20, 13,7,20, 13,8,20, 13,8,20, 13,9,19, 13,9,18, 13,9,16, 13,10,16, 13,10,
14, 13,11,13, 13,13,12, 13,13,11, 12,13,10, 11,13,10, 10,13,11, 8,13,11, 6,12,11, 5,11,12, 3,10,12, 2,10,12, 0,10,12, 0,
10,13, -1,10,14, -2,9,16, -3,9,19, -4,9,22, -6,8,26, -8,6,29, -10,6,32, -10,6,32, -8,6,32, -6,7,32, -5,9,31, -4,10,28, 
-3,10,28, -1,10,27, 0,10,27, 0,10,26, 0,10,25, 0,10,24, 0,10,23, 0,9,21, 0,9,20, 0,9,19, 0,9,18, 0,8,17, 0,8,16, 0,8,17, 
0,8,17, 0,8,18, 0,9,16, 0,8,16,
};

#if 1
void test_cfs()
{
   /*        */
  /* step 1 */
  /*        */
  char message[32];
  char buf[100];
  strcpy(message,"#1.hello world.");
  strcpy(buf,message);
  printf("step 1: %s\n", buf );
  /* End Step 1. We will add more code below this comment later */    
  /*        */
  /* step 2 */
  /*        */
  /* writing to cfs */
  char *filename = "msg_file";
  int fd_write, fd_read;
  int n;
  fd_write = cfs_open(filename, CFS_WRITE);
  if(fd_write != -1) {
    n = cfs_write(fd_write, message, sizeof(message));
    cfs_close(fd_write);
    printf("step 2: successfully written to cfs. wrote %i bytes\n", n);
  } else {
    printf("ERROR: could not write to memory in step 2.\n");
  } 
  /*        */
  /* step 3 */
  /*        */
  /* reading from cfs */
  strcpy(buf,"empty string");
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read!=-1) {
    cfs_read(fd_read, buf, sizeof(message));
    printf("step 3: %s\n", buf);
    cfs_close(fd_read);
  } else {
    printf("ERROR: could not read from memory in step 3.\n");
  }
  /*        */
  /* step 4 */
  /*        */
  /* adding more data to cfs */
  strcpy(buf,"empty string");
  strcpy(message,"#2.contiki is amazing!");
  fd_write = cfs_open(filename, CFS_WRITE | CFS_APPEND);
  if(fd_write != -1) {
    n = cfs_write(fd_write, message, sizeof(message));
    cfs_close(fd_write);
    printf("step 4: successfully appended data to cfs. wrote %i bytes  \n",n);
  } else {
    printf("ERROR: could not write to memory in step 4.\n");
  }
  /*        */
  /* step 5 */
  /*        */
  /* seeking specific data from cfs */
  strcpy(buf,"empty string");
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read != -1) {
    cfs_read(fd_read, buf, sizeof(message));
    printf("step 5: #1 - %s\n", buf);
    cfs_seek(fd_read, sizeof(message), CFS_SEEK_SET);
    cfs_read(fd_read, buf, sizeof(message));
    printf("step 5: #2 - %s\n", buf);
    cfs_close(fd_read);
  } else {
    printf("ERROR: could not read from memory in step 5.\n");
  }
  /*        */
  /* step 6 */
  /*        */
  /* remove the file from cfs */
  cfs_remove(filename);
  fd_read = cfs_open(filename, CFS_READ);
  if(fd_read == -1) {
    printf("Successfully removed file\n");
  } else {
    printf("ERROR: could read from memory in step 6.\n");
  }

}
#endif


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

#include "obex.h"
static void mas_callback(int code, void* lparam, uint16_t rparam);
static struct obex_state mas_obex_state;
static const struct obex mas_obex = 
{
  &mas_obex_state, mas_callback
};
static void mas_callback(int code, void* lparam, uint16_t rparam)
{
  switch(code)
  {
    case OBEX_CB_SEND:
      hexdump(lparam, rparam);
      break;
  }
}
static const uint8_t appparams_notify[] = 
{
  0x0e, 0x01,0x01
};
static const uint8_t MAS_TARGET[16] =
{
 0xbb, 0x58, 0x2b, 0x40, 0x42, 0x0c, 0x11, 0xdb, 0xb0, 0xde, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66
};
static const char type_notify[] = "x-bt/MAP-NotificationRegistration";

void testobex()
{
  obex_init(&mas_obex);
  obex_connect(&mas_obex, MAS_TARGET, sizeof(MAS_TARGET));

  mas_obex.state->state = 2; // hack, assume we connected

  uint8_t buf[256];
  uint8_t *ptr = obex_create_request(&mas_obex, OBEX_OP_PUT, buf);
  uint8_t Fillerbyte = 0x30;
  
  ptr = obex_header_add_bytes(ptr, OBEX_HEADER_TYPE, (uint8_t*)type_notify, strlen(type_notify));
  ptr = obex_header_add_bytes(ptr, OBEX_HEADER_APPPARMS, appparams_notify, sizeof(appparams_notify));
  ptr = obex_header_add_bytes(ptr, OBEX_HEADER_ENDBODY, &Fillerbyte, 1);
    
  obex_send(&mas_obex, buf, ptr - &buf[0]);


}

int main()
{
  memcpy(&ui_config_data, &ui_config_default, sizeof(ui_config));

  testobex();
#if 1

  cfs_coffee_format();
  test_cfs();

  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  window_init();
  status_process(EVENT_WINDOW_CREATED, 0, NULL);

  // test gesture
  gesture_init(0);
  for(int i = 0; i <sizeof(inputpoints)/sizeof(int16_t); i++)
  {
    int16_t k = inputpoints[i];
    inputpoints[i] = inputpoints[i] * 6.4;
    printf("%d(%d) ", inputpoints[i], k);
  }
  for(int i = 0; i < sizeof(inputpoints) /sizeof(int16_t)/ 3; i+=3)
  {
    gesture_processdata(&inputpoints[i * 3]);
  }

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
#endif
  SimluateRun();
}
