#ifndef _TEST_H_
#define _TEST_H_

#include "contiki.h"

uint8_t test_button(uint8_t ev, uint16_t lparam, void* rparam);
uint8_t test_motor(uint8_t ev, uint16_t lparam, void* rparam);
uint8_t test_light(uint8_t ev, uint16_t lparam, void* rparam);
uint8_t test_lcd(uint8_t ev, uint16_t lparam, void* rparam);
uint8_t test_reboot(uint8_t ev, uint16_t lparam, void* rparam);

#endif
