#include <stdint.h>
#include "pedometer.h"
#include <stdio.h>

#define SAMPLE_HZ 50

unsigned short step_cnt;
static uint16_t interval;

static int16_t last[3];

void ped_step_detect_init(void)
{
#if 0
  sample_count = 0;
  sample_new = sample_old = 0;
  min_sample = 65535;
  max_sample = 0;
  dc = 65535 / 2;
  interval = 0;
  mode = -4;
  precision = (max_sample - min_sample)/4;
#endif
  last[0] = last[1] = last[2] = 0;
}

unsigned short ped_step_detect(void)
{
  return step_cnt;
}

static int16_t filter(int16_t value, int16_t *slot)
{
  int16_t ret = value - *slot; 
  *slot = ret / 8 + *slot;
  
  return ret;
}

static int16_t filter2(int16_t value)
{
  static int32_t lastsample = 0;
  
  uint32_t ret;
  if (value > lastsample)
    ret = (value - lastsample) / 2 + lastsample;
  else
    ret = (value - lastsample) / 16 + lastsample;
  
  lastsample = ret;
  
  return ret / 2; // gt = 1/2 
}

static int16_t totalaccel(short *data)
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


char ped_update_sample(short *data)
{
 //printf("%d\t%d\t%d\t", data[0], data[1], data[2]);
#if 0
  // cacluate the total accel
  uint16_t sample = totalaccel(data);
  static uint8_t sample_count;
  static uint16_t sample_new, sample_old;
  static uint16_t min_sample, max_sample;
  static uint16_t dc;
  static uint16_t precision;
  static int8_t mode;

  if (sample > max_sample)
    max_sample = sample;
  if (sample < min_sample)
    min_sample = sample;
  
  sample_count++;
  interval++;
  
  if (sample_count == SAMPLE_HZ)
  {
    sample_count = 0;
    dc = (min_sample + max_sample)/2;
    precision = (max_sample - min_sample)/4;
    printf("min=%d max=%d dc = %d precisioin=%d\n", min_sample, max_sample, dc, precision);
    min_sample = 65535;
    max_sample = 0;
  }
  
  printf("[%d] (%d -> %d) ", sample, sample_new, sample_old);
  sample_old = sample_new;
  if (sample - sample_new < precision || sample - sample_new > -precision)
  {
    printf("\n");
    return 1;
  }
  
  // yes
  sample_new = sample;
  if (sample_old > dc && dc > sample_new)
  {
    // check the time
    if (interval > SAMPLE_HZ/5 && interval < SAMPLE_HZ * 2)
    {
      if (mode == 1)
      {
        step_cnt+=4;
        mode++;
      }
      else if (mode > 1)
      {
        step_cnt++;
      }
      else
      {
        mode++;
      }
      interval = 0;
    }
    else
    {
      mode = -4;
    }
  }
  
  printf(" %d\n", mode);
  
#else
  int16_t total = totalaccel(data); 
  int16_t delta = filter2(total);
  
  static int holdoff = 0;
  interval++;
  
  if (holdoff > 0)
  {
    holdoff--;
  }
  else if (holdoff < 0)
  {
    if (total > delta)
    {
    }
    else
    {
      holdoff = SAMPLE_HZ/5;
    }
  }
  else if (total > delta)
  {
    //if (interval > SAMPLE_HZ/5 && interval < SAMPLE_HZ * 2)
    {
      step_cnt++;
      interval = 0;
    }
    holdoff = -1;
  }

 //printf("| %d\t%d\t%d\t%d\n", total, delta, holdoff, step_cnt);
  
#endif
  return 1;
}