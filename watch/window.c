#include "contiki.h"

#include "window.h"
#include "backlight.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <stdio.h>

extern void mpu6050_init();
extern void ant_init();
extern void rtc_init();
extern void bluetooth_init();
extern void button_init();
extern void I2C_Init();

PROCESS(system_process, "System process");
AUTOSTART_PROCESSES(&system_process);

windowproc ui_window = NULL;
#define WINDOW_FLAGS_REFRESH 1
static uint8_t ui_window_flag;
static tRectangle current_clip;

const tRectangle client_clip = {0, 16, LCD_X_SIZE, LCD_Y_SIZE};

void window_init()
{
  return;
}

tContext context;

void window_open(windowproc dialog, void* data)
{
  if (ui_window)
  {
    ui_window(EVENT_WINDOW_CLOSING, 0, NULL);
  }

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
      backlight_init();
      memlcd_DriverInit();
      GrContextInit(&context, &g_memlcd_Driver);
      GrContextForegroundSet(&context, COLOR_BLACK);
      tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
      GrRectFill(&context, &rect);
      GrFlush(&context);

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


      //codec_init();
      //ant_init();
      mpu6050_init();
    }
    else if (ev == EVENT_TIME_CHANGED || ev == PROCESS_EVENT_TIMER)
    {
      // event converter to pass data as rparameter
      ui_window(ev, 0, data);
    }
    else if (ev == EVENT_KEY_PRESSED || ev == EVENT_KEY_LONGPRESSED || ev == EVENT_BT_STATUS || ev == EVENT_ANT_STATUS)
    {
      // event converter to pass data as lparam
      uint8_t ret = ui_window(ev, (uint16_t)data, NULL);

      if (!ret)
      {
        // default handler for long pressed
        if ((uint16_t)data == KEY_EXIT && ui_window != &menu_process && ev == EVENT_KEY_LONGPRESSED)
        {
          window_open(&menu_process, 0);
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

    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}

/*
* Draw the button text for the keys
* If text is NULL, draw a empty box
*/
void window_button(uint8_t key, const char* text)
{
#define SPACE 2
  uint8_t width, height;
  int x, y;

  GrContextFontSet(&context, &g_sFontNova9b);
  if (!text)
  {
    width = 100;
  }
  else
  {
    width = GrStringWidthGet(&context, text, -1);
  }

  height = GrStringHeightGet(&context);

  if (key == KEY_UP || key == KEY_DOWN)
  {
    x = LCD_X_SIZE - width - SPACE;
  }
  else
  {
    x = SPACE;
  }

  if (key == KEY_ENTER || key == KEY_DOWN)
  {
    y = 135;
  }
  else
  {
    y = 30;
  }

  // draw black box
  const tRectangle rect = {x - SPACE, y - SPACE, x + width + SPACE, y + height + SPACE};

  if (text)
  {
    GrContextForegroundSet(&context, COLOR_WHITE);
    GrRectFill(&context, &rect);
    GrContextForegroundSet(&context, COLOR_BLACK);
    GrStringDraw(&context, text, -1, x, y, 0);
  }
  else
  {
    GrContextForegroundSet(&context, COLOR_BLACK);
    GrRectFill(&context, &rect);
  }

#undef SPACE
}

void window_progress(long lY, uint8_t step)
{
  tRectangle rect = {20, lY, 125, lY + 16};
  GrContextForegroundSet(&context, COLOR_WHITE);
  GrRectFill(&context, &rect);
  GrContextForegroundSet(&context, COLOR_BLACK);

  if (step < 100)
  {
    rect.sXMin = 22;
    rect.sYMin = lY + 2;
    rect.sYMax = lY + 14;
    rect.sXMax = 22 + step;
    GrRectFill(&context, &rect);
  }
}

static struct etimer timer;

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

static windowproc notify_parent_window;
static uint8_t notify_buttons;
static uint8_t notification_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_KEY_PRESSED:
    {
      if (lparam == KEY_DOWN)
      {
        switch(notify_buttons)
        {
        case NOTIFY_OK:
          {
            process_post(&system_process, EVENT_NOTIFY_RESULT, (void*)NOTIFY_RESULT_OK);
            break;
          }
        case NOTIFY_YESNO:
          {
            process_post(&system_process, EVENT_NOTIFY_RESULT, (void*)NOTIFY_RESULT_YES);
            break;
          }
        case NOTIFY_ACCEPT_REJECT:
          {
            process_post(&system_process, EVENT_NOTIFY_RESULT, (void*)NOTIFY_RESULT_ACCEPT);
            break;
          }
          break;
        default:
          return 0;
        }
      }
      if (lparam == KEY_ENTER)
      {
        switch(notify_buttons)
        {
        case NOTIFY_YESNO:
          {
            process_post(&system_process, EVENT_NOTIFY_RESULT, (void*)NOTIFY_RESULT_NO);
            break;
          }
        case NOTIFY_ACCEPT_REJECT:
          {
            process_post(&system_process, EVENT_NOTIFY_RESULT, (void*)NOTIFY_RESULT_REJECT);
            break;
          }
          break;
        default:
          return 0;
        }
      }

      // close notification
      ui_window = notify_parent_window;
      notify_parent_window = NULL;
      break;
    }
  default:
    return 0;
  }

  return 0;
}

void window_notify(const char* message, uint8_t buttons, windowproc callback)
{
  // switch proc to windows_notify service
  notify_parent_window = ui_window;
  if (callback == NULL)
    ui_window = notification_process;
  else
    ui_window = callback;

  notify_buttons = buttons;
  // show the notification

  return;
}

void window_invalid(const tRectangle *rect)
{
  if (rect != NULL)
  {
    current_clip = *rect;
  }
  else
  {
    current_clip = client_clip;
  }

  ui_window_flag |= WINDOW_FLAGS_REFRESH;
}