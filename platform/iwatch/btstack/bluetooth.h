#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_

#define BT_INITIALIZED  BIT0
#define BT_CONNECTED    BIT1

extern void bluetooth_init();
extern void bluetooth_shutdown();
extern void bluetooth_discoverable(uint8_t onoff);
extern uint8_t bluetooth_paired();

PROCESS_NAME(bluetooth_process);

#endif