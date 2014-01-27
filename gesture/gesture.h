#ifndef _GESTURE_H_
#define _GESTURE_H_
#include <stdint.h>

void gesture_init(int8_t _recording);
void gesture_processdata(int16_t *input);
void gesture_shutdown();

#endif