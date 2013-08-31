#ifndef _SYSTEM_H_
#define _SYSTEM_H_

void system_init();

void system_rebootToNormal();
uint8_t system_testing();
uint8_t system_debug();
uint8_t system_txpower(uint8_t i);
void system_ready();

#endif