#include <stdint.h>
#include "pedometer.h"
#include <stdio.h>

#define SAMPLE_HZ 50

unsigned short step_cnt;
static uint8_t sample_count;
static uint16_t sample_new, sample_old;
static uint16_t min_sample, max_sample;
static uint16_t dc;
static uint16_t interval;
static int8_t mode; // -4 is default value, when > 0, means in confirmd mode
static uint16_t precision;

void ped_step_detect_init(void)
{
  sample_count = 0;
  sample_new = sample_old = 0;
  min_sample = 65535;
  max_sample = 0;
  dc = 65535 / 2;
  interval = 0;
  mode = -4;
  precision = (max_sample - min_sample)/16;
}

unsigned short ped_step_detect(void)
{
}

static uint16_t totalaccel(int16_t *data)
{
  uint16_t total = 0;
  // compute each axis abs
  for(int i = 0; i < 3; i++)
  {
    int16_t v = data[i];
    if (v < 0)
      v = -v;

     total += v/2;
  }
  
  return total;
}


char ped_update_sample(signed short *data)
{
  // cacluate the total accel
  uint16_t sample = totalaccel(data);
  
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
    precision = (max_sample - min_sample)/16;
    printf("min=%d max=%d dc = %d precisioin=%d\n", min_sample, max_sample, dc, precision);
    min_sample = 65535;
    max_sample = 0;    
    
  }
  
  printf("%d: [%d] (new:%d old:%d) ", sample_count, sample, sample_new, sample_old);
  sample_old = sample_new;
  if (sample - sample_new < precision || sample - sample_new > -precision)
  {
    printf("noise\n");
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
  
  printf(" mode=%d\n", mode);
  return 1;
}