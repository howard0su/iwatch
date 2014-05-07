#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "memlcd.h"
#include "backlight.h"
#include "status.h"

#include "btstack/ble/att_client.h"

#include <stdio.h>

static const char* message_title;
static const char* message_subtitle;
static const char* message;
static const char* message_date;
static char message_icon;
static uint8_t message_buttons;
static uint8_t message_result;

static uint32_t lastmessageid = -1;

#define BORDER 5

static const tRectangle contentrect = 
{0, 12, LCD_X_SIZE, LCD_Y_SIZE};

static enum
{
  STATE_ACTIVE = 0x01,
  STATE_MORE = 0x02,
  STATE_PENDING = 0x04
}state;

static int skip = 0;
static uint8_t notify_process(uint8_t ev, uint16_t lparam, void* rparam);

#define EMPTY 0xfe
#define SPECIAL 0xff

#define MAX_NOTIFY 5

static uint8_t  num_uids = 0;
static uint8_t  selectidx = 0;
static uint32_t uids[MAX_NOTIFY];
static uint32_t attributes[MAX_NOTIFY];

static void onDrawTitleBar(tContext *pContext)
{
  // draw the title bar of circles
  const tRectangle rect = {0, 0, LCD_X_SIZE, 12};

  GrContextForegroundSet(pContext, ClrBlack);
  GrContextClipRegionSet(pContext, &rect);

  GrRectFill(pContext, &rect);

  GrContextForegroundSet(pContext, ClrWhite);

  long startx = LCD_X_SIZE/2 - num_uids * 4;

  for(int i = 0; i < num_uids; i++)
  {
    if (i == selectidx)
      GrCircleFill(pContext, startx + i * 10, 4, 3);
    else
      GrCircleDraw(pContext, startx + i * 10, 4, 3);
  }
}

static const tFont *get_titlefont()
{
  switch(window_readconfig()->font_config)
  {
    case 1:
      return (const tFont*)&g_sFontGothic28b;
    case 2:
      return (const tFont*)&g_sFontUnicode;
      break;
    default:
      return (const tFont*)&g_sFontGothic24b;
  }
}

static const tFont *get_contentfont()
{
  switch(window_readconfig()->font_config)
  {
    case 1:
      return (const tFont*)&g_sFontGothic24b;
    case 2:
      return (const tFont*)&g_sFontUnicode;
    case 0:
    default:
      return (const tFont*)&g_sFontGothic18b;
  }
  
}

static void convertdate(char *buf, const char* date)
{
  // date is in format YYYYMMDDTHHMM
  int month;
  int day;

  month = (date[4] - '0') * 10 + (date[5] - '0');
  day = (date[6] - '0') * 10 + (date[7] - '0');

  sprintf(buf, "%d %s, %c%c%c%c", day, month_shortname[month - 1], date[0], date[1], date[2], date[3]);
}

static void onDraw(tContext *pContext)
{
  // draw circles
  onDrawTitleBar(pContext);

  GrContextClipRegionSet(pContext, &contentrect);
  GrContextForegroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &fullscreen_clip);

  GrContextForegroundSet(pContext, ClrBlack);

  long starty = 12 - skip;

  if (message_icon && message_date)
  {
    // draw the title bar
    // draw icon
    if (message_icon)
    {
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);    
      GrStringDraw(pContext, &message_icon, 1, 8, starty, 0);
    }

    if (message_date)
    {
      char buf[40];
      convertdate(buf, message_date);

      GrContextFontSet(pContext, (tFont*)&g_sFontGothic14);
      GrStringDraw(pContext, buf, -1, 30, starty, 0);
    }

    starty += 16;
  }

  const tFont *titleFont;
  const tFont *contentFont;

  titleFont = get_titlefont();
  contentFont = get_contentfont();

  GrContextFontSet(pContext, titleFont);

  // draw title
  if (message_title && (*message_title != '\0'))
  {
    starty = GrStringDrawWrap(pContext, message_title, 1, starty, LCD_X_SIZE - 1, 0);
  }

  if (message_subtitle && (*message_subtitle != '\0'))
    starty = GrStringDrawWrap(pContext, message_subtitle, 1, starty, LCD_X_SIZE - 1, 0);
    
  GrContextFontSet(pContext, contentFont);
  //draw message
  if (message && *message != '\0')
  {
    if (GrStringDrawWrap(pContext, message, 1, starty, LCD_X_SIZE - 1,  0) == -1)
    {
      state |= STATE_MORE;
      for(int i = 0; i < 6; i++)
      {
          GrLineDrawH(pContext, 130 - i, 130 + i,  160 - i);
      }
    }
    else
    {
      state &= ~STATE_MORE;
    }
  }
}

static void push_uid(uint32_t id, uint32_t attribute)
{
  if (num_uids <= MAX_NOTIFY - 1)
    num_uids++;

  for(int i = num_uids - 1; i >= 0; i--)
  {
    uids[i] = uids[i - 1];
    attributes[i] = attributes[i - 1];
  }

  uids[0] = id;
  attributes[0] = attribute;
}

void window_notify(const char* title, const char* msg, uint8_t buttons, char icon)
{
  message_title = title;
  message_subtitle = NULL;
  message = msg;
  message_buttons = buttons;
  message_icon = icon;
  skip = 0;

  push_uid(SPECIAL, 0);

  selectidx = 0;

  motor_on(50, CLOCK_SECOND);
  backlight_on(window_readconfig()->light_level, CLOCK_SECOND * 3);

  if (state & STATE_ACTIVE)
    window_invalid(NULL);
  else 
    window_open(notify_process, NULL);
}

void fetch_content()
{
  uint32_t uid;
  uint32_t attribute;
  message_title = NULL;
  message = NULL;

  uid = uids[selectidx];
  attribute = attributes[selectidx];

  att_fetch_next(uid, attribute);
}

void window_notify_ancs_init()
{
  lastmessageid = -1;
}

void window_notify_ancs(uint8_t command, uint32_t uid, uint8_t flag, uint8_t category)
{
  if (command == 0) // add
  {
    if (lastmessageid != -1 && lastmessageid >= uid)
    {
      return;
    }

    message_title = NULL;
    message = NULL;
    push_uid(uid, (flag << 8) | category);
    selectidx = 0;
    motor_on(50, CLOCK_SECOND);
    backlight_on(window_readconfig()->light_level, CLOCK_SECOND * 3);

    lastmessageid = uid;
    if (state & STATE_ACTIVE)
      window_invalid(NULL);
    else 
      window_open(notify_process, NULL);

    fetch_content();    
  }
  else if (command == 1)
  {
    if (state & STATE_ACTIVE)
    {
      // check if the current 
      if (uids[0] == uid)
      {
        fetch_content();
      }
      window_invalid(NULL);
      motor_on(50, CLOCK_SECOND);
    }
  }
  else if (command == 2) // remove
  {
    if (!(state & STATE_ACTIVE))
      return;

    uint8_t refresh = 0;
    if (uids[0] == uid)
    {
      refresh = 1;
    }

    // find the item
    int i;
    for(i = 0; i < num_uids; i++)
    {
      if (uids[i] == uid)
        break;
    }

    for (int j = i ; j < num_uids; j++)
    {
      uids[j] = uids[j+1];
      attributes[j] = attributes[j+1];
    }
    num_uids--;

    if (refresh)
      fetch_content();
  }
}

void window_notify_content(const char* title, const char* subtitle, const char* msg, const char* date, uint8_t buttons, char icon)
{
  message_title = title;
  message_subtitle = subtitle;
  message = msg;
  message_date = date;
  message_buttons = buttons;
  message_icon = icon;

  skip = 0;

  window_invalid(NULL);
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
  case EVENT_WINDOW_PAINT:
    {
      onDraw((tContext*)rparam);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    state &= ~STATE_ACTIVE;
    process_post(ui_process, EVENT_NOTIFY_RESULT, (void*)message_result);
    motor_on(0, 0);
    del_watch_status(WS_NOTIFY);
    selectidx = 0;
    num_uids = 0;
    return 0;
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_DOWN)
    {
      if (state & STATE_MORE)
      {
        skip += 16;
        window_invalid(NULL);
      }
      else if (selectidx < num_uids)
      {
        selectidx++;
        fetch_content();
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
      else if (skip == 0)
      {
        if (selectidx > 0)
        {  
          selectidx--;
          fetch_content();
          window_invalid(NULL);
        }
     }
    }
    else if (lparam == KEY_ENTER)
    {
      if (selectidx < num_uids - 1)
      {
        selectidx++;
        fetch_content();
        window_invalid(NULL);
      }
    }
    break;
  default:
    return 0;
  }

  return 1;
}

