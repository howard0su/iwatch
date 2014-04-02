#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "backlight.h"
#include "status.h"

#include "btstack/ble/att_client.h"

#include <stdio.h>

static const char* message_title;
static const char* message;
static char message_icon;
static uint8_t message_buttons;
static uint8_t message_result;

#define BORDER 5
#define BOTTOMBAR 0

static const tRectangle rect[5] =
{
  {0, 0, LCD_X_SIZE, BORDER},
  {0, 0, BORDER, LCD_Y_SIZE},
  {0, LCD_Y_SIZE - BORDER - BOTTOMBAR, LCD_X_SIZE, LCD_Y_SIZE - BOTTOMBAR},
  {LCD_X_SIZE - BORDER, 0, LCD_X_SIZE, LCD_Y_SIZE - BOTTOMBAR},
  {BORDER/2, BORDER/2, LCD_X_SIZE - BORDER/2, LCD_Y_SIZE - BORDER/2 - BOTTOMBAR}
};

static const tRectangle contentrect = 
{8, 26, LCD_X_SIZE - BORDER/2, LCD_Y_SIZE - BORDER/2 - BOTTOMBAR};

static enum
{
  STATE_ACTIVE = 0x01,
  STATE_MORE = 0x02,
  STATE_PENDING = 0x04
}state;

static uint8_t skip = 0;
static uint8_t notify_process(uint8_t ev, uint16_t lparam, void* rparam);


#define EMPTY 0xfe
#define SPECIAL 0xff

#define MAX_NOTIFY 5

static uint8_t  num_uids = 0;
static uint32_t uids[MAX_NOTIFY];
static uint32_t attributes[MAX_NOTIFY];

static void onDrawAlarm(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &fullscreen_clip);

  // draw table title
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFillRound(pContext, &rect[4], 4);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);

  GrContextFontSet(pContext, (tFont*)&g_sFontExIcon48);
  GrStringDrawCentered(pContext, &message_icon, 1, LCD_X_SIZE/2, LCD_Y_SIZE, 0);
}

static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &fullscreen_clip);

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);

  // draw circles

  // draw icon
  if (message_icon)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);    
    GrStringDraw(pContext, &message_icon, 1, 12, 6, 0);
  }

  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
  GrStringCodepageSet(pContext, CODEPAGE_UTF_8);

  // draw title
  if (message_title)
  {
  GrStringDraw(pContext, message_title, -1, 34, 6, 0);
  // draw the line
  GrLineDrawH(pContext, 5, LCD_X_SIZE - 4, 23);
  }


  GrContextClipRegionSet(pContext, &contentrect);
  //draw message
  if (message)
  {
    if (GrStringDrawWrap(pContext, message, 8, 26 - skip, LCD_X_SIZE - 12,  0))
    {
      state |= STATE_MORE;
    }
    else
    {
      state &= ~STATE_MORE;
    }
  }

  GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);
}

static void push_uid(uint32_t id, uint32_t attribute)
{
  for(int i = num_uids - 1; i >= 0; i--)
  {
    uids[i] = uids[i - 1];
    attributes[i] = attributes[i - 1];
  }

  uids[0] = id;
  attributes[0] = attribute;
  if (num_uids < MAX_NOTIFY)
    num_uids++;
}

static void dump_uid()
{
  for(int i = 0; i < num_uids; i++)
  {
    printf("%d uid: %ld\n", i, uids[i]);
  }
}

static void pop_uid()
{
  if (num_uids == 0)
    return;

  for(int i = 0; i < num_uids - 1 && i > 0; i++)
  {
    uids[i] = uids[i + 1];
    attributes[i] = attributes[i + 1];
  }

  num_uids--;
}

static void get_uid(uint32_t *uid, uint32_t *attribute)
{
  if (num_uids == 0)
    *uid = EMPTY;

  *uid = uids[0];
  *attribute = attributes[0];
}

static int more_uid()
{
  return num_uids > 0;
}

void window_notify(const char* title, const char* msg, uint8_t buttons, char icon)
{
  message_title = title;
  message = msg;
  message_buttons = buttons;
  message_icon = icon;

  push_uid(SPECIAL, 0);

  motor_on(50, CLOCK_SECOND);
  backlight_on(window_readconfig()->light_level, CLOCK_SECOND * 3);

  if (state & STATE_ACTIVE)
    window_invalid(NULL);
  else 
    window_open(notify_process, NULL);
}

void fetch_next()
{
  uint32_t uid;
  uint32_t combine;

  get_uid(&uid, &combine);
  att_fetch_next(uid, combine);
}

void window_notify_ancs(uint32_t uid, uint32_t combine)
{
  message_title = NULL;
  message = NULL;
  push_uid(uid, combine);

  motor_on(50, CLOCK_SECOND);
  backlight_on(window_readconfig()->light_level, CLOCK_SECOND * 3);

  if (state & STATE_ACTIVE)
    window_invalid(NULL);
  else 
    window_open(notify_process, NULL);

  fetch_next();

  dump_uid();
}

void window_notify_content(const char* title, const char* msg, uint8_t buttons, char icon)
{
  message_title = title;
  message = msg;
  message_buttons = buttons;
  message_icon = icon;

  window_invalid(NULL);

  dump_uid();
}

// notify window process
static uint8_t notify_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
  {
    state |= STATE_ACTIVE;
    add_watch_status(WS_NOTIFY);
    return 0x80;
  }
  case EVENT_WINDOW_ACTIVE:
  {
    if ((message_buttons & NOTIFY_ALARM) == NOTIFY_ALARM)
    {
      motor_on(50, 0);
    }
    // read the first id from SPI flash
    break;
  }
  case EVENT_WINDOW_PAINT:
    {
      if ((message_buttons & NOTIFY_ALARM) == NOTIFY_ALARM)
        onDrawAlarm((tContext*)rparam);
      else
        onDraw((tContext*)rparam);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    state &= ~STATE_ACTIVE;
    process_post(ui_process, EVENT_NOTIFY_RESULT, (void*)message_result);
    motor_on(0, 0);
    del_watch_status(WS_NOTIFY);
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_ENTER)
    {
      message_result = NOTIFY_RESULT_OK;
      window_close();
    }
    else if (lparam == KEY_DOWN)
    {
      if (state & STATE_MORE)
      {
        skip += 16;
        window_invalid(NULL);
      }
      else if (num_uids > 1)
      {
        pop_uid();
        fetch_next();
      }
    }
    else if (lparam == KEY_UP)
    {
      if (skip >= 16)
      {
        skip-=16;
        window_invalid(NULL);
      }
    }
    break;
  default:
    return 0;
  }

  return 1;
}
