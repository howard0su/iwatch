#include "contiki.h"

#include "window.h"
#include "backlight.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <stdio.h>
#include "dev/flash.h"

extern void mpu6050_init();
extern void ant_init();
extern void rtc_init();
extern void bluetooth_init();
extern void button_init();
extern void I2C_Init();

PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);

#define WINDOW_FLAGS_REFRESH            1
#define WINDOW_FLAGS_STATUSUPDATE       2

static uint8_t ui_window_flag = 0;
static tRectangle current_clip;

#if defined(__GNUC__)
__attribute__ ((section(".infoc")))
#else
#pragma constseg = INFOC
#endif
static const ui_config ui_config_data =
{
  UI_CONFIG_SIGNATURE
};
#ifndef __GNUC__
#pragma constseg = default
#endif

const tRectangle client_clip = {0, 17, LCD_X_SIZE, LCD_Y_SIZE};
const tRectangle status_clip = {0, 0, LCD_X_SIZE, 16};
static tContext context;
static struct etimer timer, status_timer;


// the real stack is like this
//     uistack[4]
//     uistack[3]
//     uistack[2]
//     uistack[1]
//     uistack[0] <<== stackptr = ui_window

#define MAX_STACK 6
#define ui_window (stack[stackptr])
static windowproc stack[MAX_STACK]; // assume 5 is enough
static uint8_t stackptr = 0;

void window_init()
{
  return;
}

void window_open(windowproc dialog, void* data)
{
  stackptr++;
  ui_window = dialog;
  ui_window(EVENT_WINDOW_CREATED, 0, data);

  ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
  GrContextClipRegionSet(&context, &client_clip);
  ui_window(EVENT_WINDOW_PAINT, 0, &context);
  GrFlush(&context);
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
    if (ev == PROCESS_EVENT_INIT)
    {
      window_init();
      backlight_init();
      memlcd_DriverInit();
      GrContextInit(&context, &g_memlcd_Driver);
      GrContextForegroundSet(&context, ClrBlack);
      tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
      GrRectFill(&context, &rect);

      // give time to starts
      button_init();
      rtc_init();
      I2C_Init();

      // welcome dialog depends on backlight/lcd/i2c
      //window_open(&watch_process, NULL);
      ui_window = watch_process;
      ui_window(EVENT_WINDOW_CREATED, 0, data);

      ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
      GrContextClipRegionSet(&context, &client_clip);
      ui_window(EVENT_WINDOW_PAINT, 0, &context);
      GrFlush(&context);

      status_process(EVENT_WINDOW_CREATED, 0, data);

      //codec_init();
      //ant_init();
      mpu6050_init();

      etimer_set(&status_timer, CLOCK_SECOND * 3);
    }
    else if (ev == PROCESS_EVENT_TIMER)
    {
      if (data == &timer)
      {
        ui_window(ev, 0, data);
      }
      else if (data == &status_timer)
      {
        ui_window_flag |= WINDOW_FLAGS_STATUSUPDATE;
      }
    }
    else if (ev == EVENT_TIME_CHANGED)
    {
      // event converter to pass data as rparameter
      ui_window(ev, 0, data);
    }
    else if (ev == EVENT_BT_STATUS || ev == EVENT_ANT_STATUS)
    {
      status_process(ev, (uint16_t)data, NULL);
      ui_window(ev, (uint16_t)data, NULL);
    }
    else if (ev == EVENT_KEY_PRESSED || ev == EVENT_KEY_LONGPRESSED)
    {
      // event converter to pass data as lparam
      uint8_t ret = ui_window(ev, (uint16_t)data, NULL);

      if (!ret)
      {
        // default handler for long pressed
        if ((uint16_t)data == KEY_EXIT && stackptr != 0 && (ev == EVENT_KEY_PRESSED || ev == EVENT_KEY_LONGPRESSED))
        {
          window_close();
        }
      }
    }

    if (ui_window_flag & WINDOW_FLAGS_REFRESH)
    {
      ui_window_flag &= ~WINDOW_FLAGS_REFRESH;
      GrContextClipRegionSet(&context, &current_clip);
      ui_window(EVENT_WINDOW_PAINT, 0, &context);
      GrFlush(&context);
    }

    if (ui_window_flag & WINDOW_FLAGS_STATUSUPDATE)
    {
      ui_window_flag &= ~WINDOW_FLAGS_STATUSUPDATE;
      GrContextClipRegionSet(&context, &status_clip);
      status_process(EVENT_WINDOW_PAINT, 0, &context);
      GrFlush(&context);
    }

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
    current_clip = *rect;
  }
  else
  {
    // todo, merge two rect
    current_clip = client_clip;
  }

  ui_window_flag |= WINDOW_FLAGS_REFRESH;
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
  GrContextClipRegionSet(&context, &client_clip);
  ui_window(EVENT_WINDOW_PAINT, 0, &context);
  GrFlush(&context);
}

const ui_config* window_readconfig()
{
  return &ui_config_data;
}

void window_writeconfig(ui_config* data)
{
  // write to flash
  flash_setup();
  flash_clear((uint16_t*)&ui_config_data);
  flash_writepage((uint16_t*)&ui_config_data, (uint16_t*)&data, sizeof(ui_config_data));
  flash_done();
}
