#ifndef PEDOMETER_H
#define PEDOMETER_H
#include <stdint.h>

void ped_step_detect_init(void);
unsigned short ped_step_detect(void);
char ped_update_sample(int16_t *);

uint32_t ped_get_steps();
#endif
