#include "contiki.h"
#include "backlight.h"
#include "sys/ctimer.h"

#define LIGHTDIR P4DIR
#define LIGHTOUT P4OUT
#define LIGHTSEL P4SEL
#define LIGHT    BIT2
#define LIGHTCONTROL TB0CCTL2
#define LIGHTLEVEL TB0CCR2

#define MOTORDIR P4DIR
#define MOTOROUT P4OUT
#define MOTORSEL P4SEL
#define MOTOR    BIT1
#define MOTORCONTROL TB0CCTL1
#define MOTORLEVEL TB0CCR1

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