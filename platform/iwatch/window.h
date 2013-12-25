#ifndef _WINDOW_H_
#define _WINDOW_H_
#include "grlib/grlib.h"
#include <stdint.h>

PROCESS_NAME(system_process);

// the active process to handle input and various notifications
#define ui_process (&system_process)

/* Key name for key event */
#define KEY_UP          2
#define KEY_DOWN        3
#define KEY_ENTER       0
#define KEY_EXIT        1

#define KEY_TAP         4
#define KEY_DOUBLETAP   5


// parameter is used as rparam
#define EVENT_TIME_CHANGED            0x90
#define EVENT_NOTIFICATION            0x91
#define EVENT_SPORT_DATA              0x92
#define EVENT_BT_RING                 0x93
#define EVENT_BT_CLIP                 0x94
#define EVENT_BT_CIEV                 0x95
#define EVENT_BT_BVRA                 0x96

// parameter is used as lparam
#define EVENT_WINDOW_CREATED          0xc0
#define EVENT_WINDOW_CLOSING          0xc1
#define EVENT_WINDOW_CLOSED           0xc2
#define EVENT_KEY_PRESSED             0xc3
#define EVENT_KEY_LONGPRESSED         0xc4
#define EVENT_WINDOW_ACTIVE           0xc5
#define EVENT_WINDOW_DEACTIVE         0xc6
#define EVENT_GESTURE_MATCHED         0xc7

// move exit key into another message, so any application want to handle this 
// message, have to be more explict
#define EVENT_EXIT_PRESSED            0xa0
#define EVENT_BT_STATUS               0xa1 // parameters BIT0:ENABLE, BIT1:CONNECT
#define EVENT_ANT_STATUS              0xa2 // parameters BIT0:EnABLE, BIT1:CONNECT
#define EVENT_MPU_STATUS              0xa3 // parameters BIT0:EnABLE
#define EVENT_CODEC_STATUS            0xa4 // parameters BIT0:EnABLE
#define EVENT_WINDOW_PAINT            0xa5 // no parameter
#define EVENT_NOTIFICATION_DONE       0xa6
#define EVENT_NOTIFY_RESULT           0xa7 // the notification result


typedef uint8_t (*windowproc)(uint8_t event, uint16_t lparam, void* rparam);

extern void window_init(void);
extern void window_open(windowproc proc, void* data);
extern void window_invalid(const tRectangle *rect);
extern void status_invalid(void);  // invalid status
extern void window_timer(clock_time_t time);
extern void window_close(void);
extern windowproc window_current();
extern tContext* window_context();
extern void window_postmessage(uint8_t event, uint16_t lparam, void *rparam);

// Common control
extern void window_button(tContext *pContext, uint8_t key, const char* text);
extern void window_progress(tContext *pContext, long lY, uint8_t step);
extern void window_drawtime(tContext *pContext, long y, uint8_t times[3], uint8_t selected);

#define NOTIFY_OK 0
#define NOTIFY_YESNO 1
#define NOTIFY_ACCEPT_REJECT 2

#define NOTIFY_RESULT_OK 1
#define NOTIFY_RESULT_YES 1
#define NOTIFY_RESULT_NO 2
#define NOTIFY_RESULT_ACCEPT 1
#define NOTIFY_RESULT_REJECT 2

extern void window_notify(const char* title, const char* message, uint8_t buttons, char icon);

extern const tRectangle client_clip;

extern uint8_t status_process(uint8_t event, uint16_t lparam, void* rparam);

// Dialogs
extern uint8_t analogclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t digitclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t menu_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t control_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t countdown_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t watch_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t btconfig_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t configdate_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t configtime_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t stopwatch_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t calendar_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t selftest_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t sportswatch_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t worldclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t today_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t sporttype_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t upgrade_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t phone_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t script_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t shutdown_process(uint8_t ev, uint16_t lparam, void* rparam);
extern uint8_t siri_process(uint8_t ev, uint16_t lparam, void* rparam);

#define UI_CONFIG_SIGNATURE 0xABADFACE
typedef struct {
  uint32_t signature;

  // world clock config
  char worldclock_name[6][10];
  int8_t worldclock_offset[6];

  uint8_t default_clock; // 0 - analog, 1 - digit
  // analog clock config
  uint8_t analog_clock;  // num : which clock face
  // digit clock config
  uint8_t digit_clock;   // num : which clock face

  // sports watch config
  uint8_t sports_grid;   // 0 - 3 grid, 1 - 4 grid, 2 - 5 grid
  uint8_t sports_grid_data[5]; // each slot means which grid data to show
  // default

  //goals
  uint16_t goal_steps;
  uint16_t goal_distance;
  uint16_t goal_calories;

  uint8_t weight; // in kg
  uint8_t height; // in cm

  //gesture
  //#define GESTURE_FLAG_ENABLE 0x01
  //#define GESTURE_FLAG_LEFT   0x02
  uint8_t gesture_flag;
  uint8_t gesture_map[4];

  uint8_t circumference;
}ui_config;

extern ui_config* window_readconfig();
extern void window_writeconfig();
extern void window_loadconfig();

// the const strings
extern const char * const month_name[];
extern const char * const month_shortname[];
extern const char * const week_shortname[];
extern const char* toEnglish(uint8_t number, char* buffer);

// #define EVENT_SPORT_DATA              0x92
// lparam defined as below
#define DATA_WORKOUT    0
#define DATA_SPEED      1
#define DATA_HEARTRATE  2
#define DATA_CALS       3
#define DATA_DISTANCE   4
#define DATA_SPEED_AVG  5
#define DATA_ALTITUTE   6
#define DATA_TIME       7
#define DATA_SPEED_TOP  8
#define DATA_CADENCE    9

#endif
