#ifndef PEDOMETER_H
#define PEDOMETER_H
#include <stdint.h>

void ped_step_detect_init(void);
unsigned short ped_step_detect(void);
char ped_update_sample(int16_t *);

uint16_t ped_get_steps();
uint16_t ped_get_calorie();
uint16_t ped_get_time();
uint16_t ped_get_distance();
uint16_t calc_step_len(uint16_t interval, uint8_t height);

void ped_reset();
#endif
