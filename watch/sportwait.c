#include "contiki.h"
#include "window.h"
#include "Template_Driver.h"
#include "ant/ant.h"
#include "stlv_client.h"
#include "ble_handler.h"

static void onDraw(tContext *pContext)
{
  int width;
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  // display the text
  GrStringDraw(pContext,"Wait for GPS/ANT", -1, 10, 80, 0);
}

static uint8_t selection;
static uint8_t okflags;
static uint8_t sports_type = 0;

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
        uint16_t stlv_data[6] = {0};
        uint32_t ble_data_buf[5] = {0};

        //STLV over RFCOMM
        send_sports_data(0, sports_type | SPORTS_DATA_FLAG_PRE, stlv_data, 5);

        //BLE
        uint32_t ble_desc_buf[2] = {1, 0};
        write_uint32_array(BLE_HANDLE_SPORTS_DESC, ble_desc_buf, 2);
        write_uint32_array(BLE_HANDLE_SPORTS_DATA, ble_data_buf, 5);
      }
    return 0x80;
  case EVENT_SPORT_DATA:

    //TODO: wait for GPS or ANT data based on selection
    window_close(); // close self
    window_open(&sportswatch_process, (void*)selection);
    break;
  case EVENT_WINDOW_CLOSING:
    {
      uint16_t dummy_stlv_data[4] = {0};
      send_sports_data(0, sports_type | SPORTS_DATA_FLAG_STOP, dummy_stlv_data, 4);

      uint32_t dummy_ble_buf[5] = {0};
      write_uint32_array(BLE_HANDLE_SPORTS_DATA, dummy_ble_buf, 5);
    }
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_NOTIFY_RESULT:
    window_close();
    break;
  default:
    return 0;
  }

  return 1;
}
