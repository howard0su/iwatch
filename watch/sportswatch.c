#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"

#include "rtc.h"

#define GRID_3 			0
#define GRID_4 			1
#define GRID_5 			2

#define DATA_WORKOUT	0
#define DATA_SPEED 		1
#define DATA_HEARTRATE 	2
#define DATA_CALS		3
#define DATA_DISTINCT	4
#define DATA_SPEED_AVG	5
#define DATA_ALTITUTE	6
#define DATA_TIME		7
#define DATA_SPEED_TOP	8

// the data for each grid
static uint16_t data[5];
static uint16_t workout_time;
// configuration for sport watch
// in ui_config
//  grid type
//  grid1, grid2, grid3, grid4, grid5
//    grid0 is always time spent

struct _datapoints
{
  uint8_t data;
  const char *name; // use \t to seperate the string
  const char *unit;
};

static const struct _datapoints datapoints[] =
{
  {DATA_WORKOUT, "Total Workout Time", NULL},
  {DATA_SPEED, "Speed", "mph"},
  {DATA_HEARTRATE, "Heart\tRate", "bpm"},
  {DATA_DISTINCT, "Distinct", "mile"},
  {DATA_CALS, "Burned\tCalories", "cal"},
  {DATA_SPEED_AVG, "Avg Speed", "mph"},
  {DATA_SPEED, "Speed", "mph"},
  {DATA_ALTITUTE, "Altitude", "ft"},
  {DATA_TIME, "Time of\tthe Day", NULL},
  {DATA_SPEED_TOP, "Top\tSpeed", "mph"}
};

static const tRectangle region_3grid[] =
{
  {0, 0, LCD_X_SIZE, 83},
  {0, 84, 74, LCD_Y_SIZE},
  {75, 85, LCD_X_SIZE, LCD_Y_SIZE}
};


static const tRectangle region_4grid[] =
{
  {0, 0, LCD_X_SIZE, 41},
  {0, 42, LCD_X_SIZE, 84},
  {0, 85, LCD_X_SIZE, 125},
  {0, 126, LCD_X_SIZE, LCD_Y_SIZE},
};

static const tRectangle region_5grid[] =
{
  {0, 0, LCD_X_SIZE, 41},
  {0, 42, 72, 105},

};

static const tRectangle *regions[] =
{
  region_3grid, region_4grid, region_5grid
};

static void drawGridTime(tContext *pContext)
{

}

// draw one grid data
static void drawGridData(tContext *pContext, uint8_t grid, uint16_t data)
{
  if (grid == 0)
  {
    // draw time
    drawGridTime(pContext);
    return;
  }

  // other generic grid data
  switch(window_readconfig()->sports_grid)
  {
  case GRID_3:
    break;
  case GRID_4:
    break;
  case GRID_5:
    break;
  }
}

static void onDraw(tContext *pContext)
{
  // first draw the datetime
  uint8_t totalgrid = window_readconfig()->sports_grid + 3;

  for(uint8_t g = 0; g < totalgrid; g++)
  {
    drawGridData(pContext, g, data[g]);
  }
}

// Find which grid slot contains the specific data
static int findDataGrid(uint8_t data)
{
  uint8_t totalgrid = window_readconfig()->sports_grid + 3;

  for(uint8_t g = 0; g < totalgrid; g++)
  {
    if (window_readconfig()->sports_grid_data[g] == data)
      return g;
  }

  return -1;
}

static void updateData(uint8_t datatype, uint16_t value)
{
  int slot = findDataGrid(datatype);

  if (slot == -1)
    return;

  data[slot] = value;
  // if we find the slot, invalid it
  window_invalid(&regions[window_readconfig()->sports_grid][slot]);
}

uint8_t sportswatch_process(uint8_t event, uint16_t lparam, void* rparam)
{
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      rtc_enablechange(SECOND_CHANGE);
      for (int i = 0; i < 5; i++)
        data[i] = 0;
      workout_time = 0;
      break;
    }
  case EVENT_TIME_CHANGED:
    {
      workout_time++;
      updateData(DATA_WORKOUT, workout_time);
      break;
    }
  case EVENT_WINDOW_PAINT:
    {
      tContext *pContext = (tContext*)rparam;
      GrContextForegroundSet(pContext, ClrBlack);
      GrRectFill(pContext, &client_clip);
      GrContextForegroundSet(pContext, ClrWhite);
      onDraw(pContext);
      break;
    }
  case EVENT_WINDOW_CLOSING:
    process_post(ui_process, EVENT_NOTIFY_RESULT, NULL);
    break;
  default:
    return 0;
  }

  return 1;
}