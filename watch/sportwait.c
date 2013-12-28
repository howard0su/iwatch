#include "contiki.h"
#include "window.h"
#include "Template_Driver.h"
#include "ant/ant.h"

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

uint8_t sportwait_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
      selection = (uint8_t)rparam;
      if (selection == 0)
      {
          //running
        ant_init(MODE_HRM);
      }
      else
      {
          //cycling
        ant_init(MODE_CBSC);
      }
    return 0x80;
  case EVENT_SPORT_DATA:

    //TODO: wait for GPS or ANT data based on selection
    window_close(); // close self
    window_open(&sportswatch_process, (void*)selection);
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