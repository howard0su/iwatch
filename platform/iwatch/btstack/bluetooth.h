#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#define BT_INITIALIZED  1
#define BT_CONNECTED    2
#define BT_DISCONNECTED 3
#define BT_PAIRED       4
#define BT_SHUTDOWN     5

extern void bluetooth_init();
extern void bluetooth_shutdown();
extern void bluetooth_discoverable(uint8_t onoff);
extern uint8_t bluetooth_paired();

PROCESS_NAME(bluetooth_process);

#endif