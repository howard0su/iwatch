#include "contiki.h"

#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <stdio.h>
#include <string.h>
#include "backlight.h"
#include "cfs/cfs.h"
#include "btstack/src/hfp.h"
#include "system.h"
#include "memory.h"

PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);

#define WINDOW_FLAGS_REFRESH            1
#define WINDOW_FLAGS_STATUSUPDATE       2

static uint8_t ui_window_flag = 0;
static tRectangle current_clip;

extern const unsigned char logoPixel[];
extern void filesys_init();

union _data d;

//  uint32_t signature;
//  char worldclock_name[6][10];
//  int8_t worldclock_offset[6];
//  uint8_t default_clock; // 0 - analog, 1 - digit
//  uint8_t analog_clock;  // num : which clock face
//  uint8_t digit_clock;   // num : which clock face
//  uint8_t sports_grid;   // 0 - 3 grid, 1 - 4 grid, 2 - 5 grid
//  uint8_t sports_grid_data[5]; // each slot means which grid data to show
//  uint8_t is_ukuint;
//  uint16_t goal_steps;
//  uint16_t goal_distance;
//  uint16_t goal_calories;
//  uint8_t weight; // in kg
//  uint8_t height; // in cm
//  uint8_t circumference;
//  uint8_t gesture_flag;
//  uint8_t gesture_map[4];
//  uint16_t lap_length;
static ui_config ui_config_data =
{
  UI_CONFIG_SIGNATURE, //signature
  10000, 2000, 500, //goals
  400, //lan_length

  {"Shanghai", "London", "New York", "Place A", "Place B", "Place C",}, //worldclock_name
  {+16, +8, +3, +1, +2, -5,}, //worldclock_offset

  0, //default_clock
  4, //analog_clock
  2, //digit_clock

  1, //sports_grid
  { 0, 1, 2, 3, 4, }, // sports_grid_data
  0x00, //is_ukunit
  60, 170, 82, //profiles
  0x00, { 0, 1, 2, 3, }, //gesture

  8, 8 // default level of volume and light
};



const tRectangle client_clip = {0, 17, LCD_X_SIZE, LCD_Y_SIZE};
const tRectangle status_clip = {0, 0, LCD_X_SIZE, 16};
const tRectangle fullscreen_clip = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
static tContext context;
static struct etimer timer, status_timer, backlight_timer;

// the real stack is like this
//     uistack[4]
//     uistack[3]
//     uistack[2]
//     uistack[1]
//     uistack[0] <<== stackptr = ui_window

#define MAX_STACK 10
#define ui_window (stack[stackptr])
static windowproc stack[MAX_STACK]; // assume 6 is enough
static uint16_t statusflag = 0; // max 16
static uint8_t stackptr = 0;

void window_init()
{
  backlight_on(255);
  filesys_init();

  current_clip = client_clip;
  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  printf("WIN: Initialize...");
  GrContextForegroundSet(&context, ClrBlack);
  tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
  GrRectFill(&context, &rect);

  GrContextForegroundSet(&context, ClrWhite);
  GrImageDraw(&context, logoPixel, 8, 60);

  GrFlush(&context);
  stackptr = 0;
  //window_open(&menu_process, NULL);
  ui_window = menu_process;
  printf("Done\n");
  return;
}

void window_open(windowproc dialog, void* data)
{
  ui_window(EVENT_WINDOW_DEACTIVE, 0, NULL);

  stackptr++;
  ui_window = dialog;
  if (ui_window(EVENT_WINDOW_CREATED, 0, data) == 0x80)
  {
    // special case, no status window
    statusflag |= 1 << stackptr;
  }

  if (((statusflag & (1 << stackptr)) != 0) ^ (((statusflag & (1 << (stackptr -1))) != 0)))
  {
    status_invalid();
  }

  ui_window(EVENT_WINDOW_ACTIVE, 0, NULL);

  window_invalid(NULL);
}

static int window_isstatusoff()
{
  return (statusflag & (1 << stackptr));
}

static void window_handle_event(uint8_t ev, void* data)
{
    if (ev == PROCESS_EVENT_INIT)
    {
      backlight_on(0);
      CFSFontWrapperLoad();

      window_loadconfig();

      // continue create menu window
      ui_window(EVENT_WINDOW_CREATED, 0, NULL);
      ui_window(EVENT_WINDOW_ACTIVE, 0, NULL);

      ui_window_flag |= WINDOW_FLAGS_REFRESH;
      status_process(EVENT_WINDOW_CREATED, 0, data);

      etimer_set(&status_timer, CLOCK_SECOND * 10);
    }
    else if (ev == PROCESS_EVENT_TIMER)
    {
      if (data == &status_timer)
      {
        status_process(ev, 0, data);
        etimer_set(&status_timer, CLOCK_SECOND * 10);
      }
      else if (data == &backlight_timer)
      {
        backlight_on(0);
      }
      else
      {
        ui_window(ev, 0, data);
      }
    }
    else if (ev == EVENT_TIME_CHANGED)
    {
      // event converter to pass data as rparameter
      ui_window(ev, 0, data);
    }
    else if (ev == EVENT_BT_STATUS || ev == EVENT_ANT_STATUS || ev == EVENT_AV)
    {
      system_ready();
      status_process(ev, (uint16_t)data, NULL);
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_BT_CIEV)
    {
      uint16_t d = (uint16_t)data;
      if (window_current() != &phone_process &&
          window_current() != &siri_process &&
          (d == 0x0302 || d==0x0301 || d == 0x0303))
      {
        window_open(&phone_process, NULL);
      }
      else
      {
        ui_window(ev, (uint16_t)data, NULL);
      }
    }
    else if (ev == EVENT_BT_BVRA)
    {
      if (window_current() != &siri_process && data)
      {
        window_open(&siri_process, NULL);
      }
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_BT_CLIP || ev == EVENT_BT_RING || ev == EVENT_BT_BVRA)
    {
      ui_window(ev, 0, data);
    }
    else if (ev == EVENT_NOTIFY_RESULT || ev == EVENT_GESTURE_MATCHED)
    {
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_KEY_PRESSED || ev == EVENT_KEY_LONGPRESSED)
    {
      backlight_on(window_readconfig()->light_level);
      etimer_set(&backlight_timer, CLOCK_SECOND * 3);

      if (ev == EVENT_KEY_PRESSED && (uint16_t)data == KEY_EXIT)
      {
        uint8_t ret = ui_window(EVENT_EXIT_PRESSED, 0, NULL);
        if (!ret)
            window_close();
      }
      else if (ev == EVENT_KEY_LONGPRESSED && (uint16_t)data == KEY_ENTER)
      {
        // switch to phone call interface to show Siri
        window_open(&siri_process, (void*)1);
      }
      else if (EVENT_KEY_LONGPRESSED && (uint16_t)data == KEY_EXIT)
      {
        window_close();
      }
      else
      {
        // event converter to pass data as lparam
        ui_window(ev, (uint16_t)data, NULL);
      }
    }
    else if (ev == EVENT_FILESYS_LIST_FILE)
    {
        ui_window(ev, 0, data);
    }

    // check if there is more message in the queue
    if (!process_moreevent(&system_process))
    {
      if (ui_window_flag & WINDOW_FLAGS_STATUSUPDATE)
      {
        ui_window_flag &= ~WINDOW_FLAGS_STATUSUPDATE;
        
        GrContextClipRegionSet(&context, &status_clip);
        if (window_isstatusoff())
        {
          GrContextForegroundSet(&context, ClrBlack);
          GrRectFill(&context, &status_clip);
        }
        else
        {
          GrContextForegroundSet(&context, ClrWhite);
          status_process(EVENT_WINDOW_PAINT, 0, &context);
        }
      }

      if (ui_window_flag & WINDOW_FLAGS_REFRESH)
      {
        ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
        GrContextForegroundSet(&context, ClrWhite);
        printf("update %d, %d %d, %d\n", current_clip.sXMin, current_clip.sYMin, current_clip.sXMax, current_clip.sYMax);
        GrContextClipRegionSet(&context, &current_clip);
        ui_window(EVENT_WINDOW_PAINT, 0, &context);
        current_clip.sXMin = 255;
        current_clip.sXMax = 0;
        current_clip.sYMin = 255;
        current_clip.sYMax = 0;
      }

      GrFlush(&context);
    }
}

/*
* This process is the startup process.
* It first shows the logo
* Like the whole dialog intrufstture.
*/
PROCESS_THREAD(system_process, ev, data)
{
  PROCESS_BEGIN();

  while(1)
  {
    window_handle_event(ev, data);
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

void window_timer(clock_time_t time)
{
  if (time == 0)
  {
    etimer_stop(&timer);
  }
  else
  {
    etimer_set(&timer, time);
  }
}

void window_invalid(const tRectangle *rect)
{
  if (rect != NULL)
  {
    if (rect->sXMin < current_clip.sXMin)
      current_clip.sXMin = rect->sXMin;
    if (rect->sYMin < current_clip.sYMin)
      current_clip.sYMin = rect->sYMin;
    if (rect->sXMax > current_clip.sXMax)
      current_clip.sXMax = rect->sXMax;
    if (rect->sYMax > current_clip.sYMax)
      current_clip.sYMax = rect->sYMax;
  }
  else
  {
    if (window_isstatusoff())
    {
      current_clip = fullscreen_clip;
    }
    else
    {
      current_clip = client_clip; 
    }
  }

  ui_window_flag |= WINDOW_FLAGS_REFRESH;
  process_post(ui_process, EVENT_WINDOW_PAINT, NULL);
}

void status_invalid()
{
  if (!window_isstatusoff())
  ui_window_flag |= WINDOW_FLAGS_STATUSUPDATE;
}

void window_close()
{
  if (stackptr == 0)
    return;

  ui_window(EVENT_WINDOW_CLOSING, 0, NULL);
  
  if (((statusflag & (1 << stackptr)) != 0) ^ (((statusflag & (1 << (stackptr -1))) != 0)))
  {
    status_invalid();
  }

  statusflag &= ~(1 << stackptr);
  stackptr--;
  ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
  ui_window(EVENT_WINDOW_ACTIVE, 0, NULL);
  current_clip.sXMin = 255;
  current_clip.sXMax = 0;
  current_clip.sYMin = 255;
  current_clip.sYMax = 0;
//  ui_window(EVENT_WINDOW_PAINT, 0, &context);
//  GrFlush(&context);
  window_invalid(NULL);
  status_invalid();
}

#define WINDOWCONFIG "_uiconfig"

void window_loadconfig()
{
  ui_config data;
  printf("load config file\n");
  int fd = cfs_open(WINDOWCONFIG, CFS_READ);
  if (fd != -1)
  {
    int length = cfs_read(fd, &data, sizeof(data));
    cfs_close(fd); 
  
    if (length == sizeof(data) && (data.signature == UI_CONFIG_SIGNATURE))
    {
      // valid config
      memcpy(&ui_config_data, &data, sizeof(data));
    }
    else
    {
      // if invalid file, flush current
      window_writeconfig();      
    }
  }
}

ui_config* window_readconfig()
{
  return &ui_config_data;
}

void window_writeconfig()
{
  int fd = cfs_open(WINDOWCONFIG, CFS_WRITE);
  if (fd == -1)
  {
    printf("error write config\n");
    return;
  }
  cfs_write(fd, &ui_config_data, sizeof(ui_config_data));
  cfs_close(fd);
}

windowproc window_current()
{
  return ui_window;
}

tContext* window_context()
{
  return &context;
}

/*
 * call window procedure sync
 */
void window_postmessage(uint8_t event, uint16_t lparam, void *rparam)
{
  PROCESS_CONTEXT_BEGIN(&system_process);
  ui_window(event, lparam, rparam);
  PROCESS_CONTEXT_END(&system_process);
}
