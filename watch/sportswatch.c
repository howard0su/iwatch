#include "contiki.h"
#include "window.h"

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

static uint16_t seconds;
static uint8_t time[3];

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

static const struct _datapoints workouttime = 
{
	DATA_WORKOUT, "Total Workout Time", NULL
};

static const struct _datapoints datapoints[] = 
{
	{DATA_SPEED, "Speed", "mph"},
	{DATA_HEARTRATE, "Heart\tRate", "bpm"},
	{DATA_DISTINCT, "Distinct", "mile"},
	{DATA_CALS, "Burned\tCalories", "cal"},
	{DATA_SPEED_AVG, "Avg Speed", "mph"},
	{DATA_SPEED, "Speed", "mph"}
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

static void drawData(struct _datapoints *datapoints, uint16_t data)
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

// Find which grid slot contains the specific data
static int findDataGrid(uint8_t data)
{
	for()
}

static void updateData()
{

}

int sportwatch_process(uint8_t event, uint16_t lparam, void* rparam)
{
	switch(event)
	{
		case EVENT_WINDOW_CREATED:

			rtc_enable(SECOND_CHANGE);
			break;
		case EVENT_WINDOW_CLOSING:
			break;

		default:
			return 0;
	}

	return 1;
}