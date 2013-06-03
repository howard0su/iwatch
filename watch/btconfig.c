#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

#include "btstack/bluetooth.h"

static enum {BT_ON, BT_OFF, BT_INITIALING} state;
PROCESS_NAME(bluetooth_process);

void draw_screen(tContext *pContext)
{
    // initialize state
  GrContextFontSet(pContext, &g_sFontNova16);

  // clear the region
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextBackgroundSet(pContext, ClrBlack);

  // display text
  if (state == BT_ON)
  {
    GrStringDraw(pContext, "Bluetooth is on", -1, 10, 68, 0);
    GrContextFontSet(pContext, &g_sFontNova16);
    GrStringDraw(pContext, "device is disconverable now.", -1, 20, 90, 0);

    window_button(pContext, KEY_DOWN, "OFF");
  }
  else if (state == BT_OFF)
  {
    GrStringDraw(pContext, "Bluetooth is off", -1, 10, 68, 0);
    window_button(pContext, KEY_DOWN, "ON");
  }
  else
  {
    GrStringDraw(pContext, "Bluetooth is initializing", -1, 10, 68, 0);
    //todo: add init timeout
  }

  if (bluetooth_paired())
  {
    GrStringDraw(pContext, "Device is paired", -1, 10, 50, 0);
  }

  GrFlush(pContext);
}

uint8_t btconfig_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev)
  {
  case EVENT_WINDOW_CREATED:
    {
      // check the btstack status
      if (process_is_running(&bluetooth_process))
      {
        state = BT_ON;

        // if btstack is on, make it discoverable
        bluetooth_discoverable(1);
      }
      else
      {
        state = BT_OFF;
      }

      return 1;
    }
  case EVENT_WINDOW_PAINT:
    {
      draw_screen((tContext*)rparam);
      return 1;
    }
  case EVENT_BT_STATUS:
    {
      if ((lparam == BT_INITIALIZED) && (state == BT_OFF || state == BT_INITIALING))
      {
        bluetooth_discoverable(1);
        state = BT_ON;
      }
      else if ((lparam == BT_SHUTDOWN) && state == BT_ON)
      {
        state = BT_OFF;
      }

      window_invalid(NULL);
      return 1;
    }
  case EVENT_KEY_PRESSED:
    {
      if (lparam == KEY_DOWN)
      {
        if (state == BT_ON)
        {
          bluetooth_shutdown();
          state = BT_OFF;
        }
        else if (state == BT_OFF)
        {
          state = BT_INITIALING;
          bluetooth_init();
        }
        window_invalid(NULL);
        return 1;
      }
      break;
    }
  case EVENT_WINDOW_CLOSING:
    {
      if (state == BT_ON)
      {
        if (bluetooth_paired())
        {
          bluetooth_discoverable(0);
        }
      }
      break;
    }
  }

  return 0;
}