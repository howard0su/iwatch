#ifndef _WINDOW_H_
#define _WINDOWN_H_

// the active process to handle input and various notifications
extern struct process *ui_process;

/* Key name for key event */
#define KEY_UP          0
#define KEY_DOWN        1
#define KEY_ENTER       2
#define KEY_EXIT        3


#define EVENT_WINDOW_CREATED          0x90
#define EVENT_WINDOW_CLOSING          0x91
#define EVENT_WINDOW_CLOSED           0x92
#define EVENT_KEY_PRESSED             0x93
#define EVENT_KEY_LONGPRESSED         0x94
#define EVENT_TIME_CHANGED            0x95
#define EVENT_BT_STATUS               0x96 // parameters BIT0:ENABLE, BIT1:CONNECT
#define EVENT_ANT_STATUS              0x97 // parameters BIT0:EnABLE, BIT1:CONNECT
#define EVENT_MPU_STATUS              0x98 // parameters BIT0:EnABLE
#define EVENT_CODEC_STATUS            0x99 // parameters BIT0:EnABLE
#define EVENT_WINDOW_PAINT            0xa0 // no parameter

typedef uint8_t (*windowproc)(uint8_t event, uint16_t lparam, void* rparam);

extern void window_init();
extern void window_open(windowproc proc, void* data);
extern void window_button(uint8_t key, const char* text);

// Dialogs
extern uint8_t analogclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t digitclock_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t menu_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t control_process(uint8_t event, uint16_t lparam, void* rparam);
extern uint8_t countdown_process(uint8_t event, uint16_t lparam, void* rparam);
#endif