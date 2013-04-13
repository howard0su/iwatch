#ifndef _WINDOW_H_
#define _WINDOW_H_

PROCESS_NAME(system_process);

// the active process to handle input and various notifications
#define ui_process (&system_process)

/* Key name for key event */
#define KEY_UP          2
#define KEY_DOWN        3
#define KEY_ENTER       0
#define KEY_EXIT        1


// parameter is used as rparam
#define EVENT_TIME_CHANGED            0x90

// parameter is used as lparam
#define EVENT_WINDOW_CREATED          0xa0
#define EVENT_WINDOW_CLOSING          0xa1
#define EVENT_WINDOW_CLOSED           0xa2
#define EVENT_KEY_PRESSED             0xa3
#define EVENT_KEY_LONGPRESSED         0xa4
#define EVENT_BT_STATUS               0xa6 // parameters BIT0:ENABLE, BIT1:CONNECT
#define EVENT_ANT_STATUS              0xa7 // parameters BIT0:EnABLE, BIT1:CONNECT
#define EVENT_MPU_STATUS              0xa8 // parameters BIT0:EnABLE
#define EVENT_CODEC_STATUS            0xa9 // parameters BIT0:EnABLE
#define EVENT_WINDOW_PAINT            0xaa // no parameter

typedef uint8_t (*windowproc)(uint8_t event, uint16_t lparam, void* rparam);

extern void window_init();
extern void window_open(windowproc proc, void* data);
extern void window_button(uint8_t key, const char* text);
extern void window_progress(long lY, uint8_t step);
extern void window_timer(clock_time_t time);

// Dialogs
extern uint8_t analogclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t digitclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t menu_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t control_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t countdown_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t watch_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t btconfig_process(uint8_t event, uint16_t lparam, void* rparam);
#endif