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
#define BOTTOMBAR 25

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

static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrContextBackgroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &fullscreen_clip);

  // draw table title
  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);
  GrRectFillRound(pContext, &rect[4], 4);

  GrContextForegroundSet(pContext, ClrBlack);

  // draw icon
  if (message_icon)
  {
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);    
    GrStringDraw(pContext, &message_icon, 1, 12, 6, 0);
  }

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

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextFontSet(pContext, (tFont*)&g_sFontUnicode);
  GrStringCodepageSet(pContext, CODEPAGE_UTF_8);

  // draw title
  GrStringDraw(pContext, message_title, -1, 34, 6, 0);
  // draw the line
  GrLineDrawH(pContext, 5, 126, 23);

  GrContextClipRegionSet(pContext, &contentrect);
  //draw message
  GrStringDrawWrap(pContext, message, 8, 26, LCD_X_SIZE - 26,  16);

  GrStringCodepageSet(pContext, CODEPAGE_ISO8859_1);
}

// notify window process
static uint8_t notify_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
  {
    return 0x80;
  }
  case EVENT_WINDOW_PAINT:
    {
      onDraw((tContext*)rparam);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    process_post(ui_process, EVENT_NOTIFY_RESULT, (void*)message_result);
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_ENTER)
    {
      message_result = NOTIFY_RESULT_OK;
      window_close();
    }
    else if ((lparam == KEY_DOWN) && (message_buttons != NOTIFY_OK))
    {
      message_result = NOTIFY_RESULT_NO;
      window_close();
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

  if (window_current() == notify_process)
    window_invalid(NULL);
  else 
    window_open(notify_process, NULL);
}

