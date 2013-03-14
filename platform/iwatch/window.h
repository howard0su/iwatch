#ifndef _WINDOW_H_
#define _WINDOWN_H_

// the active process to handle input and various notifications
extern struct process *ui_process;

#define EVENT_WINDOW_CREATED          0x90
#define EVENT_WINDOW_CLOSING          0x91
#define EVENT_WINDOW_CLOSED           0x92
#define EVENT_KEY_PRESSED             0x93
#define EVENT_KEY_LONGPRESSED         0x94

/* Key name for key event */
#define KEY_UP          0
#define KEY_DOWN        1
#define KEY_ENTER       2
#define KEY_EXIT        3

#define EVENT_TIME_CHANGED            0x95

#define EVENT_BT_STATUS               0x96 // parameters BIT0:ENABLE, BIT1:CONNECT
#define EVENT_ANT_STATUS              0x97 // parameters BIT0:EnABLE, BIT1:CONNECT
#define EVENT_MPU_STATUS              0x98 // parameters BIT0:EnABLE
#define EVENT_CODEC_STATUS            0x99 // parameters BIT0:EnABLE

// Dialogs
PROCESS_NAME(analogclock_process);
PROCESS_NAME(digitclock_process);
PROCESS_NAME(menu_process);
PROCESS_NAME(control_process);

extern void window_defproc(process_event_t ev, process_data_t data);
extern void window_init();
extern void window_showdialog(struct process* dialog, void* data);
extern void window_open(struct process* dialog, void* data);
#endif