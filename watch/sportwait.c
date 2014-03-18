#include "contiki.h"
#include "window.h"
#include "Template_Driver.h"
#include "ant/ant.h"
#include "stlv_client.h"
#include "ble_handler.h"

static uint8_t selection;
static uint8_t okflags;
static uint8_t sports_type = 0;

static void onDraw(tContext *pContext)
{
  int width;
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &fullscreen_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  // display the text
  GrStringDraw(pContext,"Wait for GPS/ANT", -1, 10, 80, 0);
  if (sports_type == SPORTS_DATA_FLAG_RUN)
    window_button(pContext, KEY_ENTER, " IGNORE");
}

uint8_t sportwait_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
      selection = (uint8_t)rparam;
      if (selection == 0)
      {
        //running
        sports_type = SPORTS_DATA_FLAG_RUN;
        ant_init(MODE_HRM);
      }
      else
      {
        //cycling
        sports_type = SPORTS_DATA_FLAG_BIKE;
        ant_init(MODE_CBSC);
      }

      //send out start workout data to watch
      {
        uint8_t stlv_data[6] = {0};
        uint8_t ble_data_buf[5] = {0};

        //STLV over RFCOMM
        send_sports_data(0, sports_type | SPORTS_DATA_FLAG_PRE, stlv_data, 5);

        //BLE
        ble_start_sync(1);
      }
    return 0x80;
  case EVENT_SPORT_DATA:
    window_close(); // close self
    window_open(&sportswatch_process, (void*)selection);
    break;
  case EVENT_WINDOW_CLOSING:
    {
      uint8_t dummy_stlv_data[4] = {0};
      send_sports_data(0, sports_type | SPORTS_DATA_FLAG_STOP, dummy_stlv_data, 4);

      ble_stop_sync();
    }
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_NOTIFY_RESULT:
    window_close();
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_ENTER)
    {
      window_close(); // close self
      window_open(&sportswatch_process, (void*)selection);
    }
  default:
    return 0;
  }

  return 1;
}
