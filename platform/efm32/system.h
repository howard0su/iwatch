#ifndef _SYSTEM_H_
#define _SYSTEM_H_

void system_init();

void system_rebootToNormal();
uint8_t system_testing();
uint8_t system_retail();
void system_ready();
void system_shutdown(int shipping);
void system_restore();
void system_rebootToNormal();
void system_reset();
const char* system_getserial();
uint8_t system_locked();
void system_unlock();
void system_resetfactory();
#endif