#include "contiki.h"

#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <stdio.h>
#include <string.h>
#include "backlight.h"
#include "dev/flash.h"

PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);

#define WINDOW_FLAGS_REFRESH            1
#define WINDOW_FLAGS_STATUSUPDATE       2

static uint8_t ui_window_flag = 0;
static tRectangle current_clip;

#define INFOC (uint16_t*)0x1880

static const ui_config ui_config_default =
{
  UI_CONFIG_SIGNATURE,

  "Shanghai", "London", "New York", "", "", "",
  +16, +8, +3, +1, +2, -5,

  0,
  4,
  2,

  1,
  0, 1, 2, 3, 4
};

ui_config ui_config_data;

const tRectangle client_clip = {0, 17, LCD_X_SIZE, LCD_Y_SIZE};
const tRectangle status_clip = {0, 0, LCD_X_SIZE, 16};
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
static uint8_t stackptr = 0;
extern tImage g_logoImage;

void window_init()
{
  backlight_on(255);
  current_clip = client_clip;
  memlcd_DriverInit();
  GrContextInit(&context, &g_memlcd_Driver);
  GrContextForegroundSet(&context, ClrBlack);
  tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
  GrRectFill(&context, &rect);

  GrContextForegroundSet(&context, ClrWhite);
  GrImageDraw(&context, &g_logoImage, 0, 60);

  GrFlush(&context);
  stackptr = 0;
  //window_open(&menu_process, NULL);
  ui_window = menu_process;

  return;
}

void window_open(windowproc dialog, void* data)
{
  ui_window(EVENT_WINDOW_DEACTIVE, 0, NULL);

  stackptr++;
  ui_window = dialog;
  ui_window(EVENT_WINDOW_CREATED, 0, data);
  ui_window(EVENT_WINDOW_ACTIVE, 0, NULL);

  window_invalid(NULL);
}

void window_handle_event(uint8_t ev, void* data)
{
    if (ev == PROCESS_EVENT_INIT)
    {
      backlight_on(0);

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
    else if (ev == EVENT_TIME_CHANGED || ev == EVENT_ANT_DATA)
    {
      // event converter to pass data as rparameter
      ui_window(ev, 0, data);
    }
    else if (ev == EVENT_BT_STATUS || ev == EVENT_ANT_STATUS)
    {
#if 0
      if (!(SFRRPCR & SYSNMI))
      {
        SFRRPCR |= (SYSRSTRE + SYSRSTUP + SYSNMI);
        SFRIE1 &= ~NMIIE;
      }
#endif
      status_process(ev, (uint16_t)data, NULL);
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_RING)
    {
      if ((uint16_t)data == 0x0201) // callsetup = 1
      {
        window_open(&phone_process, NULL);
      }
      else
      {
        ui_window(ev, (uint16_t)data, NULL);
      }
    }
    else if (ev == EVENT_NOTIFY_RESULT || ev == EVENT_GESTURE_MATCHED)
    {
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_KEY_PRESSED || ev == EVENT_KEY_LONGPRESSED)
    {
      backlight_on(255);
      etimer_set(&backlight_timer, CLOCK_SECOND * 3);

      if (ev == EVENT_KEY_PRESSED && (uint16_t)data == KEY_EXIT)
      {
          uint8_t ret = ui_window(EVENT_EXIT_PRESSED, 0, NULL);
          if (!ret)
        window_close();
      }
      else
      {
        // event converter to pass data as lparam
        ui_window(ev, (uint16_t)data, NULL);
      }
    }

    // check if there is more message in the queue
    if (!process_moreevent(&system_process))
    {
      if (ui_window_flag & WINDOW_FLAGS_REFRESH)
      {
        ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
        GrContextForegroundSet(&context, ClrWhite);
        GrContextClipRegionSet(&context, &current_clip);
        ui_window(EVENT_WINDOW_PAINT, 0, &context);
        current_clip.sXMin = 255;
        current_clip.sXMax = 0;
        current_clip.sYMin = 255;
        current_clip.sYMax = 0;
      }

      if (ui_window_flag & WINDOW_FLAGS_STATUSUPDATE)
      {
        ui_window_flag &= ~WINDOW_FLAGS_STATUSUPDATE;
        GrContextForegroundSet(&context, ClrWhite);
        GrContextClipRegionSet(&context, &status_clip);
        status_process(EVENT_WINDOW_PAINT, 0, &context);
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
    current_clip = client_clip;
  }

  ui_window_flag |= WINDOW_FLAGS_REFRESH;
  process_post(ui_process, EVENT_WINDOW_PAINT, NULL);
}

void status_invalid()
{
  ui_window_flag |= WINDOW_FLAGS_STATUSUPDATE;
}

void window_close()
{
  if (stackptr == 0)
    return;

  ui_window(EVENT_WINDOW_CLOSING, 0, NULL);

  stackptr--;
  ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
  GrContextForegroundSet(&context, ClrWhite);
  GrContextClipRegionSet(&context, &client_clip);
  ui_window(EVENT_WINDOW_ACTIVE, 0, NULL);
  ui_window(EVENT_WINDOW_PAINT, 0, &context);
  GrFlush(&context);
}

CASSERT(sizeof(ui_config) <= 128, ui_config_less_than_infoc);

ui_config* window_readconfig()
{
  if (ui_config_data.signature != UI_CONFIG_SIGNATURE)
  {
    memcpy(&ui_config_data, INFOC, sizeof(ui_config_data));

    // still not valid?
    if (ui_config_data.signature != UI_CONFIG_SIGNATURE)
    {
      memcpy(&ui_config_data, &ui_config_default, sizeof(ui_config_data));
    }
  }

  return &ui_config_data;
}

void window_writeconfig()
{
  // write to flash
  flash_setup();
  flash_clear(INFOC);
  flash_writepage(INFOC, (uint16_t*)&ui_config_data, 128);
  flash_done();
}

windowproc window_current()
{
  return ui_window;
}