#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include "ant/ant.h"
#include <stdio.h>

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

#define FORMAT_NONE 0
#define FORMAT_TIME 1

struct _datapoints
{
  uint8_t data;
  const char *name; // use \t to seperate the string
  const char *unit;
  uint8_t format;
};

static const struct _datapoints datapoints[] =
{
  {DATA_WORKOUT, "Total Workout Time", NULL},
  {DATA_SPEED, "Speed", "mph"},
  {DATA_HEARTRATE, "Heart Rate", "bpm"},
  {DATA_DISTINCT, "Distinct", "mile"},
  {DATA_CALS, "Burned Calories", "cal"},
  {DATA_SPEED_AVG, "Avg Speed", "mph"},
  {DATA_SPEED, "Speed", "mph"},
  {DATA_ALTITUTE, "Altitude", "ft"},
  {DATA_TIME, "Time of the Day", NULL},
  {DATA_SPEED_TOP, "Top Speed", "mph"}
};

static const tRectangle region_3grid[] =
{
  {0, 16, LCD_X_SIZE, 83},
  {0, 84, 74, LCD_Y_SIZE},
  {75, 85, LCD_X_SIZE, LCD_Y_SIZE}
};


static const tRectangle region_4grid[] =
{
  {0, 16, LCD_X_SIZE, 44},
  {0, 45, LCD_X_SIZE, 84},
  {0, 85, LCD_X_SIZE, 124},
  {0, 125, LCD_X_SIZE, LCD_Y_SIZE},
};

static const tRectangle region_5grid[] =
{
  {0, 16, LCD_X_SIZE, 44},
  {0, 45, 72, 104},
  {72, 45, LCD_X_SIZE, 104},
  {0, 105, 72, LCD_Y_SIZE},
  {72, 105, LCD_X_SIZE, LCD_Y_SIZE},
};

static const tRectangle *regions[] =
{
  region_3grid, region_4grid, region_5grid
};

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

static void drawGridTime(tContext *pContext)
{
  char buf[20];
  uint8_t time[3];
  time[0] = workout_time % 60;
  time[1] = (workout_time / 60 ) % 60;
  time[2] = workout_time / 3600;

  GrContextForegroundSet(pContext, ClrBlack);
  switch(window_readconfig()->sports_grid)
  {
  case GRID_3:
    GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
    GrStringDraw(pContext, "i", 1, 12, 20, 0);
    GrContextFontSet(pContext, &g_sFontNova13);
    GrStringDraw(pContext, datapoints[0].name, -1, 30, 20, 0);
    GrContextFontSet(pContext, &g_sFontNova28b);
    sprintf(buf, "%02d:%02d:%02d", time[2], time[1], time[0]);
    GrStringDraw(pContext, buf, -1, 12, 40, 0);
    break;
  case GRID_4:
  case GRID_5:
    GrContextFontSet(pContext, &g_sFontNova28b);
    sprintf(buf, "%02d:%02d:%02d", time[2], time[1], time[0]);
    GrStringDraw(pContext, buf, -1, 12, 18, 0);
    break;
  }
}

// draw one grid data
static void drawGridData(tContext *pContext, uint8_t grid, uint16_t data)
{
  int width;
  if (grid == 0)
  {
    // draw time
    drawGridTime(pContext);
    return;
  }

  int index = findDataGrid(grid);
  if (index == -1)
    return;
  struct _datapoints const *d = &datapoints[index];

  char buf[20];
  // todo: format the input data
  switch(d->format)
  {
    case FORMAT_TIME:
      sprintf(buf, "%02d:%02d", data/60, data%60);
      break;
    default:
      sprintf(buf, "%d", data);
      break;
  }

  GrContextForegroundSet(pContext, ClrWhite);
  // other generic grid data
  switch(window_readconfig()->sports_grid)
  {
  case GRID_3:
    GrContextFontSet(pContext, &g_sFontNova13);
    GrStringDraw(pContext, d->name, -1,  region_3grid[index].sXMin + 8, region_3grid[index].sYMin, 0);
    GrContextFontSet(pContext, &g_sFontNova28b);
    GrStringDraw(pContext, buf, -1, region_3grid[index].sXMin + 8, region_3grid[index].sYMin + 20, 0);
    if (d->unit)
    {
      GrContextFontSet(pContext, &g_sFontNova13);
      GrStringDraw(pContext, d->unit, -1, region_3grid[index].sXMin + 8, region_3grid[index].sYMin + 50, 0);
    }
    break;
  case GRID_4:
    GrContextFontSet(pContext, &g_sFontRed13);
    GrStringDrawWrap(pContext, d->name, region_4grid[index].sXMin + 8, region_4grid[index].sYMin + 8,
                    (region_4grid[index].sXMax - region_4grid[index].sXMin) / 3, 16);

    GrContextFontSet(pContext, &g_sFontNova28b);
    width = GrStringWidthGet(pContext, buf, -1);
    GrStringDraw(pContext, buf, -1, (region_4grid[index].sXMax - region_4grid[index].sXMin) * 3 / 4 - width,
      region_4grid[index].sYMin + 10, 0);
    if (d->unit)
    {
      GrContextFontSet(pContext, &g_sFontRed13);
      GrStringDraw(pContext, d->unit, -1, (region_4grid[index].sXMax - region_4grid[index].sXMin) * 3 / 4  + 8, region_4grid[index].sYMin + 20, 0);
    }
    break;
  case GRID_5:
    GrContextFontSet(pContext, &g_sFontNova13);
    GrStringDraw(pContext, d->name, -1,  region_5grid[index].sXMin + 8, region_5grid[index].sYMin, 0);
    GrContextFontSet(pContext, &g_sFontNova28b);
    width = GrStringWidthGet(pContext, buf, -1);
    GrStringDraw(pContext, buf, -1, region_5grid[index].sXMax - 4 - width, region_5grid[index].sYMin + 15, 0);
    if (d->unit)
    {
      GrContextFontSet(pContext, &g_sFontNova13);
      GrStringDraw(pContext, d->unit, -1, region_5grid[index].sXMin + 8, region_5grid[index].sYMin + 40, 0);
    }
    break;
  }
}

static void onDraw(tContext *pContext)
{
  // first draw the datetime
  uint8_t totalgrid = window_readconfig()->sports_grid + 3;

  GrContextForegroundSet(pContext, ClrWhite);
  switch(window_readconfig()->sports_grid)
  {
  case GRID_3:
    GrRectFill(pContext, &region_3grid[0]);
    GrLineDrawV(pContext, region_3grid[1].sXMax, region_3grid[1].sYMin, region_3grid[1].sYMax);
    break;
  case GRID_4:
    GrRectFill(pContext, &region_4grid[0]);
    GrLineDrawH(pContext, region_4grid[1].sXMin, region_4grid[1].sXMax, region_4grid[1].sYMax);
    GrLineDrawH(pContext, region_4grid[2].sXMin, region_4grid[2].sXMax, region_4grid[2].sYMax);
    break;
  case GRID_5:
    GrRectFill(pContext, &region_5grid[0]);
    GrLineDrawH(pContext, region_5grid[1].sXMin, region_5grid[2].sXMax, region_5grid[1].sYMax);
    GrLineDrawV(pContext, region_5grid[1].sXMax, region_5grid[1].sYMin, region_5grid[3].sYMax);
    break;
  }

  GrContextForegroundSet(pContext, ClrWhite);
  for(uint8_t g = 0; g < totalgrid; g++)
  {
    drawGridData(pContext, g, data[g]);
  }
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
      if (rparam == (void*)0)
      {
        ant_init(MODE_HRM);
      }
      else
      {
        ant_init(MODE_CBSC);
      }
      rtc_enablechange(SECOND_CHANGE);
      for (int i = 0; i < 5; i++)
        data[i] = 0;
      workout_time = 0;
      break;
    }
  case EVENT_ANT_DATA:
    {
      printf("got ant data\n");
      updateData(DATA_HEARTRATE, (uint16_t)rparam);
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
    rtc_enablechange(0);
    ant_shutdown();
    break;
  default:
    return 0;
  }

  return 1;
}