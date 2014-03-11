#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "backlight.h"

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
static uint8_t notification_ids[4];

static uint8_t lastid;

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

  // draw table title
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFillRound(pContext, &rect[4], 4);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);

  // draw icon
  if (message_icon)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);    
    GrStringDraw(pContext, &message_icon, 1, 12, 6, 0);
  }

#if 0
  switch(message_buttons)
  {
  case NOTIFY_OK:
    window_button(pContext, KEY_ENTER, "OK");
    break;
  case NOTIFY_YESNO:
    window_button(pContext, KEY_ENTER, "Yes");
    window_button(pContext, KEY_DOWN, "No");
    break;
  case NOTIFY_ACCEPT_REJECT:
    window_button(pContext, KEY_ENTER, "Accept");
    window_button(pContext, KEY_DOWN, "Reject");
    break;
  }
#endif

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
  GrStringCodepageSet(pContext, CODEPAGE_UTF_8);

  // draw title
  GrStringDraw(pContext, message_title, -1, 34, 6, 0);
  // draw the line
  GrLineDrawH(pContext, 5, LCD_X_SIZE - 4, 23);

  GrContextClipRegionSet(pContext, &contentrect);
  //draw message
  if (GrStringDrawWrap(pContext, message, 8, 26 - skip, LCD_X_SIZE - 12,  16))
  {
    state |= STATE_MORE;
  }
  else
  {
    state &= ~STATE_MORE;
  }

  GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);
}

// notify window process
static uint8_t notify_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
  {
    state |= STATE_ACTIVE;
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

void window_notify(const char* title, const char* msg, uint8_t buttons, char icon)
{
  message_title = title;
  message = msg;
  message_buttons = buttons;
  message_icon = icon;
  motor_on(50, CLOCK_SECOND);
  backlight_on(window_readconfig()->light_level, CLOCK_SECOND * 3);

  if (state & STATE_ACTIVE)
    window_invalid(NULL);
  else 
    window_open(notify_process, NULL);
}

