#include "contiki.h"
#include "backlight.h"
#include "sys/ctimer.h"

void backlight_init()
{
  LIGHTDIR |= LIGHT;
  LIGHTSEL |= LIGHT;

  MOTORSEL |= MOTOR;
  MOTORDIR |= MOTOR;

  TB0CTL |= TBSSEL_1 + MC_1;
  TB0CCR0 = 16; // control PWM freq = 32768/16 = 2048hz

  MOTORCONTROL = OUTMOD_0;
  LIGHTCONTROL = OUTMOD_0;
}

void backlight_shutdown()
{
  MOTORCONTROL = OUTMOD_0;
  LIGHTCONTROL = OUTMOD_0;
}


void backlight_on(uint8_t level)
{
  if (level > 16) level = 16;

  if (level == 0)
  {
    LIGHTCONTROL = OUTMOD_0;
  }
  else
  {
    LIGHTCONTROL = OUTMOD_7;
    LIGHTLEVEL = level;
  }
}

static struct ctimer motor_timer;
void motor_stop(void *ptr)
{
  MOTORCONTROL = OUTMOD_0;
}

void motor_on(uint8_t level, clock_time_t length)
{
  if (level > 16) level = 16;
  if (level == 0)
  {
    motor_stop(NULL);
  }
  else
  {
    MOTORCONTROL = OUTMOD_7;
    MOTORLEVEL = level;
    if (length > 0)
      ctimer_set(&motor_timer, length, motor_stop, NULL);
  }
}
