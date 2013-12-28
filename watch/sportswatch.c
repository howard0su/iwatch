#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include "ant/ant.h"
#include "stlv.h"
#include "stlv_client.h"
#include "ble_handler.h"
#include <stdio.h>
#include <cfs/cfs.h>
#include "btstack/include/btstack/utils.h"

#define GRID_3 			0
#define GRID_4 			1
#define GRID_5 			2

#define GPS_TIMEOUT     30

// The basic data to generate others
//    SPORTS_TIME = 0,
//    SPORTS_STEPS,
//    SPORTS_SPEED,
//    SPORTS_ALT,
//    SPORTS_HEARTRATE,
//    SPORTS_DISTANCE,
static uint32_t base_data[SPORTS_DATA_MAX] = {0};

// the data for each grid
static uint32_t grid_data[5];
static uint32_t workout_time;
// configuration for sport watch
// in ui_config
//  grid type
//  grid1, grid2, grid3, grid4, grid5
//    grid0 is always time spent

#define FORMAT_NONE 0
#define FORMAT_TIME 1
#define FORMAT_SPD  2

struct _datapoints
{
  const char *name; // use \t to seperate the string
  const char *unit;
  const uint8_t format;
};

static const struct _datapoints datapoints[] =
{
  {"Total Workout Time", NULL,   FORMAT_NONE},
  {"Speed",              "mph",  FORMAT_SPD},
  {"Heart Rate",         "bpm",  FORMAT_NONE},
  {"Burned Calories",    "cal",  FORMAT_NONE},
  {"Distance",           "mile", FORMAT_NONE},
  {"Avg Speed",          "mph",  FORMAT_NONE},
  {"Altitude",           "ft",   FORMAT_NONE},
  {"Time of the Day",    NULL,   FORMAT_TIME},
  {"Top Speed",          "mph",  FORMAT_SPD},
  {"Cadence",            "cpm",  FORMAT_NONE},
  {"Pace",               NULL,   FORMAT_TIME},
  {"Avg. Heart Rate",    "bpm",  FORMAT_NONE},
  {"Top Heart Rate",     "bpm",  FORMAT_NONE},
  {"Elevation Gain",     "ft",   FORMAT_NONE},
  {"Current Lap",        NULL,   FORMAT_TIME},
  {"Best Lap",           NULL,   FORMAT_TIME},
  {"Floors",             "cpm",  FORMAT_NONE},
  {"Steps",              NULL,   FORMAT_NONE},
  {"Avg. Pace",          NULL,   FORMAT_TIME},
  {"Avg. Lap",           NULL,   FORMAT_TIME},
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

static int upload_data_interval = 3;

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
  struct _datapoints const *d = &datapoints[grid];

  char buf[20];
  // todo: format the input data
  switch(d->format)
  {
    case FORMAT_TIME:
      sprintf(buf, "%02d:%02d", data/60, data%60);
      break;
    case FORMAT_SPD:
      sprintf(buf, "%d.%01d", data/100, data%100);
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
    drawGridData(pContext, window_readconfig()->sports_grid_data[g] , grid_data[g]);
  }
}

static void updateDistance(uint32_t value, uint32_t* gris_mask)
{
    uint16_t lap_len = window_readconfig()->lap_length;
    if (lap_len > 50)
    {
      uint32_t oldval = base_data[SPORTS_DISTANCE] % lap_len;
      uint32_t newval = value % lap_len;
      if (oldval > newval)
      {
          uint32_t lap_time = workout_time - base_data[SPORTS_TIME_LAP_BEGIN];
          if (lap_time < base_data[SPORTS_LAP_BEST])
          {
              base_data[SPORTS_LAP_BEST] = lap_time;
              (*gris_mask) |= (1 << GRID_LAPTIME_BEST);
          }
          base_data[SPORTS_TIME_LAP_BEGIN] = workout_time;
      }
    }
}

static void updateHeartRate(uint32_t value, uint32_t* gris_mask)
{
    base_data[SPORTS_TIME_LAST_HEARTRATE] = workout_time;

    if (value > base_data[SPORTS_HEARTRATE_MAX])
    {
        base_data[SPORTS_HEARTRATE_MAX] = value;
        (*gris_mask) |= (1 << GRID_HEARTRATE_MAX);
    }

    if (workout_time > 0)
    {
        base_data[SPORTS_HEARTRATE_AVG] = (
          base_data[SPORTS_HEARTRATE_AVG] * base_data[SPORTS_TIME_LAST_HEARTRATE] +
          value * (workout_time - base_data[SPORTS_TIME_LAST_HEARTRATE])
          ) / workout_time;
        (*gris_mask) |= (1 << GRID_HEARTRATE_AVG);
    }

}

static void updateSpeed(uint32_t value, uint32_t* gris_mask)
{
    if (value > base_data[SPORTS_SPEED_MAX])
    {
        base_data[SPORTS_SPEED_MAX] = value;
        (*gris_mask) |= (1 << GRID_SPEED_TOP);
    }
}

static void updateAlt(uint32_t value, uint32_t* gris_mask)
{
    if ((int)base_data[SPORTS_ALT_START] > 1000000)
        base_data[SPORTS_ALT_START] = value;
}

void updateSteps(uint32_t value, uint32_t* gris_mask)
{
    if (base_data[SPORTS_SPEED] == 0 ||
        base_data[SPORTS_TIME_LAST_GPS] == 0 ||
        workout_time - base_data[SPORTS_TIME_LAST_GPS] > 10)
    {
        //uint16_t time_interval = workout_time - base_data[SPORTS_TIME_LAST_PED];
        //if (value > base_data[SPORTS_STEPS] && time_interval >= 10)
        //{
        //    uint16_t interval = (value - base_data[SPORTS_LAST_STEPS]) * 2 / time_interval;
        //    uint16_t step_len = calc_step_len(interval);
        //    uint16_t distance = (value - base_data[SPORTS_LAST_STEPS]) * step_len;
        //    uint16_t speed    = distance / time_interval;

        //    base_data[SPORTS_TIME_LAST_PED] = workout_time;
        //    base_data[SPORTS_LAST_STEPS]    = value;
        //    updateSpeed(speed);
        //    updateSpeed(base_data[SPORTS_DISTANCE] + distance);
        //}
    }
}

static uint32_t updateBaseData(uint8_t datatype, uint32_t value)
{
  if (datatype >= SPORTS_DATA_MAX)
      return 0;

  if (datatype != 0)
    printf("updateBaseData(%u, %u)\n", datatype, value);

  uint32_t grids_mask = 0;
  switch (datatype)
  {
      case SPORTS_TIME:
          grids_mask |= (1 << GRID_WORKOUT);
          base_data[datatype] = value; //the raw value must be updated at last
          break;

      case SPORTS_DISTANCE:
          updateDistance(value, &grids_mask);
          grids_mask |= (1 << GRID_DISTANCE);
          base_data[SPORTS_TIME_LAST_GPS] = workout_time;
          base_data[datatype] += value; //the raw value must be updated at last
          break;

      case SPORTS_HEARTRATE:
          updateHeartRate(value, &grids_mask);
          grids_mask |= (1 << GRID_HEARTRATE);
          base_data[datatype] = value; //the raw value must be updated at last
          break;

      case SPORTS_SPEED:
          if (value != 0xffffffff)
          {
               updateSpeed(value, &grids_mask);
               grids_mask |= (1 << GRID_SPEED);
               base_data[SPORTS_TIME_LAST_GPS] = workout_time;
               base_data[datatype] = value; //the raw value must be updated at last
          }
          break;

      case SPORTS_ALT:
          if (value != 0xffffffff)
          {
              updateAlt(value, &grids_mask);
              grids_mask |= (1 << GRID_ALTITUTE);

              base_data[SPORTS_TIME_LAST_GPS] = workout_time;
              base_data[datatype] = value; //the raw value must be updated at last
          }
          break;

      case SPORTS_STEPS:
          updateSteps(value, &grids_mask);
          grids_mask |= (1 << GRID_STEPS);
          base_data[datatype] = value; //the raw value must be updated at last
          break;

      default:
          base_data[datatype] = value; //the raw value must be updated at last
          break;
  }
  return grids_mask;
}

static uint32_t getGridDataItem(uint8_t slot)
{
    static uint8_t s_grid_data_map[] = {
        SPORTS_TIME,
        SPORTS_SPEED,
        SPORTS_HEARTRATE,
        SPORTS_DATA_MAX, //GRID_CALS
        SPORTS_DISTANCE,
        SPORTS_DATA_MAX, //GRID_SPEED_AVG
        SPORTS_ALT,
        SPORTS_DATA_MAX, //GRID_TIME
        SPORTS_SPEED_MAX,
        SPORTS_DATA_MAX, //GRID_CADENCE
        SPORTS_DATA_MAX, //GRID_PACE
        SPORTS_HEARTRATE_AVG,
        SPORTS_HEARTRATE_MAX,
        SPORTS_DATA_MAX, //GRID_ELEVATION
        SPORTS_DATA_MAX, //GRID_LAPTIME_CUR
        SPORTS_LAP_BEST,
        SPORTS_DATA_MAX, //GRID_FLOORS
        SPORTS_STEPS,
        SPORTS_DATA_MAX, //GRID_PACE_AVG
        SPORTS_DATA_MAX, //GRID_LAPTIME_AVG
    };

    uint8_t sport_data_id = s_grid_data_map[slot];
    printf("sport_data_id=%d\n", sport_data_id);
    if (sport_data_id != SPORTS_DATA_MAX)
        return base_data[sport_data_id];

    switch (slot)
    {
        case GRID_CALS:
            break;

        case GRID_SPEED_AVG:
            if (workout_time > 0)
                return base_data[SPORTS_DISTANCE] / workout_time;
            break;


        case GRID_PACE:
            if (base_data[SPORTS_SPEED] > 0)
                return 1000 / base_data[SPORTS_SPEED];
            break;

        case GRID_ELEVATION:
            if (base_data[SPORTS_ALT_START] < 1000000)
                return base_data[SPORTS_ALT] - base_data[SPORTS_ALT_START];
            break;

        case GRID_LAPTIME_CUR:
            if (base_data[SPORTS_SPEED] > 0)
                return window_readconfig()->lap_length / base_data[SPORTS_SPEED];
            break;

        case GRID_PACE_AVG:
            if (base_data[SPORTS_DISTANCE] > 0)
                return 1000 * workout_time / base_data[SPORTS_DISTANCE];
            break;

        case GRID_LAPTIME_AVG:
            if (base_data[SPORTS_DISTANCE] > 0)
                return window_readconfig()->lap_length * workout_time /  base_data[SPORTS_DISTANCE];
            break;

        case GRID_TIME:
        case GRID_CADENCE:
        case GRID_FLOORS:
            break;
    }
    return 0;
}

static void updateGridData(uint32_t mask)
{
    ui_config* config = window_readconfig();

    for (int j = 0; j < GRID_MAX; ++j)
    {
        if ((mask & (1 << j)) == 0)
            continue;

        int slot = findDataGrid(j);
        if (slot != -1)
        {
            grid_data[slot] = getGridDataItem(j);
            if (j != 0)
                printf("updateGridData(id=%s):%d\n", datapoints[j].name, grid_data[slot]);
            window_invalid(&regions[config->sports_grid][slot]);
        }
    }
}

static void updateData(uint8_t datatype, uint32_t value)
{
    uint32_t grid_mask = updateBaseData(datatype, value);
    updateGridData(grid_mask);

//  int slot = findDataGrid(datatype);
//  if (slot == -1)
//    return;
//
//  data[slot] = value;
//  // if we find the slot, invalid it
//  window_invalid(&regions[window_readconfig()->sports_grid][slot]);
}

static uint8_t fileidx, sportnum;
static int fileid;
static uint16_t entrycount;
static uint8_t sports_type = 0;
uint8_t sportswatch_process(uint8_t event, uint16_t lparam, void* rparam)
{
  UNUSED_VAR(lparam);
  switch(event)
  {
  case EVENT_WINDOW_CREATED:
    {
      if (rparam == (void*)0)
      {
          //running
          sports_type = SPORTS_DATA_FLAG_RUN;
        //ant_init(MODE_HRM);
      }
      else
      {
          //cycling
          sports_type = SPORTS_DATA_FLAG_BIKE;
        //ant_init(MODE_CBSC);
      }
      rtc_enablechange(SECOND_CHANGE);
      for (int i = 0; i < (int)(sizeof(grid_data)/sizeof(grid_data[0])); i++)
        grid_data[i] = 0;
      for (int i = 0; i < (int)(sizeof(base_data)/sizeof(base_data[0])); i++)
        base_data[i] = 0;
      base_data[SPORTS_ALT_START] = 2000000; //set to height no possible on earth to indicate invalid data

      fileid = -1;
      workout_time = 0;
      fileidx = 0;

      ui_config* config = window_readconfig();
      sportnum = config->sports_grid + 4;

      return 0x80; // disable status
    }
  case EVENT_SPORT_DATA:
    {
      printf("got a sport data \n");
      updateData(lparam, (uint32_t)rparam);
      break;
    }
  case EVENT_TIME_CHANGED:
    {
      workout_time++;
      updateData(SPORTS_TIME, workout_time);

      if (upload_data_interval > 0 &&
          workout_time % upload_data_interval == 0)
      {
        // send data to phone
        uint16_t stlv_data[6] = {0};
        uint32_t ble_data_buf[5] = {0};
        for(int i = 0; i < sportnum; i++)
        {
            stlv_data[i]    = htons((short)(grid_data[i]));
            ble_data_buf[i] = stlv_data[i];
        }

        //STLV over RFCOMM
        send_sports_data(0, sports_type | SPORTS_DATA_FLAG_START, stlv_data, sportnum);

        //BLE
        uint32_t ble_desc_buf[2] = {1, 0};
        write_uint32_array(BLE_HANDLE_SPORTS_DESC, ble_desc_buf, 2);
        write_uint32_array(BLE_HANDLE_SPORTS_DATA, ble_data_buf, 5);

      }
#if 0
      // push the data into CFS
      if (fileid == -1)
      {
        // open the file
        char filename[32];
        sprintf(filename, "/sports/%03d.bin", fileidx++);
        cfs_remove(filename);
        fileid = cfs_open(filename, CFS_WRITE | CFS_APPEND);

        if (fileid == -1)
        {
          printf("Error to open a new file to write\n");
          break;
        }

        entrycount = 0;

        // append header
        uint16_t signature = 0x1517;
        ui_config* config = window_readconfig();
        cfs_write(fileid, &signature, sizeof(signature));
        cfs_write(fileid, &config->sports_grid, sizeof(config->sports_grid));
        sportnum = config->sports_grid + 3;
        cfs_write(fileid, config->sports_grid_data, sportnum * sizeof(config->sports_grid_data[0]));
      }

      // write the file
      if (fileid != -1)
      {
        cfs_write(fileid, &workout_time, sizeof(workout_time));
        cfs_write(fileid, data, sportnum * sizeof(data[0]));
        entrycount++;

        // if enough size, let's trucate current file and restart
        if (entrycount > 1800) // 6 * 2 = 12 bytes per entry. 30mins
        {
          cfs_close(fileid);
          fileid = -1;
        }

      }
#endif

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
    {
      if (fileid != -1)
      {
        cfs_close(fileid);
        fileid = -1;
      }
      rtc_enablechange(0);
      ant_shutdown();

      uint16_t dummy_stlv_data[4] = {0};
      send_sports_data(0, sports_type | SPORTS_DATA_FLAG_STOP, dummy_stlv_data, 4);

      uint32_t dummy_ble_buf[5] = {0};
      write_uint32_array(BLE_HANDLE_SPORTS_DATA, dummy_ble_buf, 5);
      break;
    }
  default:
    return 0;
  }

  return 1;
}
