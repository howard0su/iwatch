#include <stdint.h>
#include "pedometer.h"
uint16_t ped_get_steps()
{
  return step_cnt;
}

uint16_t ped_get_calorie()
{
  return 0;
}
uint16_t ped_get_time()
{
  return 0;
}
uint16_t ped_get_distance()
{
  return 0;
}
uint16_t calc_step_len(uint16_t interval, uint8_t height)
{
  return 0;
}

void ped_reset()
{
ped_step_detect_init();
}

