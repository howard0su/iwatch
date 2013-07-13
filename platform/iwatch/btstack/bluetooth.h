#ifndef _BLUETOOTH_H_
#define _BLUETOOTH_H_
#include "btstack/utils.h"

#define BT_INITIALIZED  1
#define BT_CONNECTED    2
#define BT_DISCONNECTED 3
#define BT_PAIRED       4
#define BT_SHUTDOWN     5

extern void bluetooth_init();
extern void bluetooth_shutdown();
extern void bluetooth_discoverable(uint8_t onoff);
extern uint8_t bluetooth_paired();
extern bd_addr_t* bluetooth_paired_addr();
extern const char* bluetooth_address();

extern void codec_setvolume(uint8_t level);
extern void codec_wakeup();
extern void codec_init();
extern void codec_shutdown();

PROCESS_NAME(bluetooth_process);

#endif