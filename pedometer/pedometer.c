#include "contiki.h"
#include <stdint.h>
#include "pedometer.h"
#include <stdio.h>
#include "window.h"
#define SAMPLE_HZ 50

static uint16_t step_cnt;
static uint32_t step_time;
static uint32_t step_cal;
static uint32_t step_dist;

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
  *slot = ret / 8 + *slot;
  
  return ret;
}

static inline int32_t filter2(int32_t value)
{
  static int32_t lastsample = 0;
  
  uint32_t ret;
  if (value > lastsample)
    ret = (value - lastsample) / 2  + lastsample;
  else
    ret = (value - lastsample) / 16 + lastsample;
  
  lastsample = ret;
  
  return ret / 2; // gt = 1/2 
}

static uint32_t totalaccel(int16_t *data)
{
  uint32_t total = 0;
  
  // compute each axis abs
  for(int i = 0; i < 3; i++)
  {
    int16_t v = filter(data[i], &last[i]);
    if (v < 0)
      v = -v;
    
    total += v;
  }
  
  return total;
}

static void sendinformation(uint16_t cmpersec)
{
  if (window_current() == sportswatch_process)
  {
    window_postmessage(EVENT_SPORT_DATA, SPORTS_PED_SPEED, (void*)(cmpersec));
    window_postmessage(EVENT_SPORT_DATA, SPORTS_PED_DISTANCE, (void*)(ped_get_distance()));
    window_postmessage(EVENT_SPORT_DATA, SPORTS_STEPS, (void*)(ped_get_steps()));
    window_postmessage(EVENT_SPORT_DATA, SPORTS_PED_CALORIES, (void*)(ped_get_calorie()));
  }
}

uint16_t calc_step_len(uint16_t interval, uint8_t height)
{
    uint16_t dist;
    if (interval < SAMPLE_HZ * 2)
        return 0;

    if (interval > SAMPLE_HZ)
    {
      dist = height / 5;
    }
    else if (interval > SAMPLE_HZ  * 2 / 3)
    {
      dist = height / 4;
    }
    else if (interval > SAMPLE_HZ / 2)
    {
      dist = height / 3;
    }
    else if (interval > SAMPLE_HZ * 2/ 5)
    {
      dist = height / 2;
    }
    else if (interval > SAMPLE_HZ / 3)
    {
      dist = height * 5 / 6;
    }
    else if (interval > SAMPLE_HZ / 4)
    {
      dist = height;
    }
    else
    {
      dist = height * 6 / 5;
    }

    printf("speed= %d\n", dist * SAMPLE_HZ / interval);
    sendinformation(dist * SAMPLE_HZ / interval);
    return dist;
}

static void increasestep(uint16_t interval)
{
  step_cnt++;

  if (interval < SAMPLE_HZ * 2)
  {
    ui_config* config = window_readconfig();
    step_time += interval;
    uint16_t dist =  calc_step_len(interval, config->height);

    step_dist += dist;
    step_cal += dist * config->weight * 5 / 4;
  }
}

char ped_update_sample(int16_t *data)
{
#if 0
  static int16_t window[4][3];
  static uint8_t lastptr;

  lastptr++;
  lastptr &= 0x03;

  window[lastptr][0] = data[0] >> 2;
  window[lastptr][1] = data[1] >> 2;
  window[lastptr][2] = data[2] >> 2;

  data[0] = window[0][0] + window[1][1] + window[2][0] + window[3][0];
  data[1] = window[0][1] + window[1][1] + window[2][1] + window[3][1];
  data[2] = window[0][2] + window[1][2] + window[2][2] + window[3][2];
#endif
  uint32_t total = totalaccel(data);
  uint16_t threshold = filter2(total);
  
  static int holdoff = 0;

  //printf("%d, %d, %d, %d\t%d\t", data[0], data[1], data[2], total, threshold);

  interval++;
  
  if (threshold < 100)
  {
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
    increasestep(interval);
    interval = 0;
    holdoff = -1;
  }

  //printf("%ld\t%d\n", threshold, step_cnt);
  
  return 1;
}

uint16_t ped_get_steps()
{
  return step_cnt;
}

uint16_t ped_get_calorie()
{
  return (uint16_t)(step_cal / 100 / 1000);
}

uint16_t ped_get_distance()
{
  return (uint16_t)(step_dist / 100);
}

uint16_t ped_get_time()
{
  return (uint16_t)(step_time / SAMPLE_HZ / 60);
}

void ped_reset()
{
  step_cnt = 0;
  step_time = 0;
  step_cal = 0;
  step_dist = 0;
}
