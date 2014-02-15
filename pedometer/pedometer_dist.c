#include "contiki.h"
#include <stdint.h>
#include "pedometer.h"
#include "window.h"
#include <stdio.h>

#define SAMPLE_HZ 50

static uint16_t last_step_cnt = 0, step_count;
static uint32_t step_time;
static uint32_t step_cal;
static uint32_t step_dist;
static uint16_t interval = 0;

uint16_t ped_get_steps()
{
  return last_step_cnt;
}

uint16_t ped_get_calorie()
{
  return (uint16_t)(step_cal / 100 / 1000);
}

uint16_t ped_get_time()
{
  return (uint16_t)(step_time / SAMPLE_HZ / 60);
}

uint16_t ped_get_distance()
{
  return (uint16_t)(step_dist / 100);
}

void ped_reset()
{
  last_step_cnt = 0;
  step_time = 0;
  step_cal = 0;
  step_dist = 0;
  interval = 0;
  ped_step_detect_init();
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

    //printf("speed= %d\n", dist * SAMPLE_HZ / interval);
    sendinformation(dist * SAMPLE_HZ / interval);
    return dist;
}

void ped_step_detect_run()
{
  step_count = ped_step_detect();
  //printf("steps: %d, %d (%d)\n", step_count, last_step_cnt, interval);
  if (interval < 65500)
      interval += 26;
  
  // if this is a step over
  if (step_count > last_step_cnt)
  {
    int steps = step_count - last_step_cnt;
    interval /= steps;
    // add workout time, must be half second, 
    if (interval > SAMPLE_HZ)
    {
      interval = SAMPLE_HZ;
    }

    step_time += interval * steps;

    // add the distance
    ui_config* config = window_readconfig();

    uint16_t dist =  calc_step_len(interval, config->height);

    step_dist += dist;
    step_cal += dist * config->weight * 5 / 4;
    
    last_step_cnt = step_count;
    interval = 0;
  }
  else if (step_count < last_step_cnt)
  {
    last_step_cnt = step_count;
  }
}
