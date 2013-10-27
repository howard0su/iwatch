#include <stdint.h>
#include "pedometer.h"
#include <stdio.h>

#define SAMPLE_HZ 50

uint32_t step_cnt;
static uint16_t interval;

static int16_t last[3];

void ped_step_detect_init(void)
{
  last[0] = last[1] = last[2] = 0;
}

unsigned short ped_step_detect(void)
{
  return step_cnt;
}

static inline int16_t filter(int16_t value, int16_t *slot)
{
  int16_t ret = value - *slot; 
  *slot = ret >> 3 + *slot;
  
  return ret;
}

static inline int16_t filter2(int16_t value)
{
  static int16_t lastsample = 0;
  
  uint16_t ret;
  if (value > lastsample)
    ret = (value - lastsample) >> 1 + lastsample;
  else
    ret = (value - lastsample) >> 4 + lastsample;
  
  lastsample = ret;
  
  return ret >> 1; // gt = 1/2 
}

static int16_t totalaccel(int16_t *data)
{
  int16_t total = 0;
  
  // compute each axis abs
  for(int i = 0; i < 3; i++)
  {
    int16_t v = filter(data[i], &last[i]);
    if (v < 0)
      v = -v;
    
    total += v/2;
  }
  
  return total;
}

char ped_update_sample(int16_t *data)
{
  int16_t total = totalaccel(data);
  int16_t threshold = filter2(total);
  
  static int holdoff = 0;

  printf("%d, %d, %d, ", data[0], data[1], data[2]);

  interval++;
  
  if (threshold < 2000)
  {
    //putchar('\n');
    return 1;
  } 

  if (holdoff > 0)
  {
    holdoff--;
  }
  else if (holdoff < 0)
  {
    if (total > threshold)
    {
    }
    else
    {
      holdoff = 1;
    }
  }
  else if (total > threshold)
  {
    step_cnt++;
    interval = 0;
    holdoff = -1;
  }

  //printf("%d\t%d\t%d\t%ld\n", total, threshold, holdoff, step_cnt);
  
  return 1;
}

uint32_t ped_get_steps()
{
  return step_cnt;
}

uint32_t ped_get_calari()
{
  return 0;
}

uint32_t ped_get_distance()
{
  return 0;
}