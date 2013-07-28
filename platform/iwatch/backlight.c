#include "contiki.h"
#include "backlight.h"
#include "sys/ctimer.h"

#define LIGHTDIR P8DIR
#define LIGHTOUT P8OUT
#define LIGHTSEL P8SEL
#define LIGHT1   BIT6
#define LIGHT2   BIT5

#define MOTORDIR P4DIR
#define MOTOROUT P4OUT
#define MOTORSEL P4SEL
#define MOTOR    BIT1

void backlight_init()
{
  LIGHTDIR |= LIGHT1 + LIGHT2;
  LIGHTOUT &= ~(LIGHT1 + LIGHT2);

  //MOTOROUT &= ~(MOTOR);
  MOTORSEL |= MOTOR;
  MOTORDIR |= MOTOR;
}

void backlight_on(uint8_t level)
{
  if (level == 0)
  {
//    TA1CTL = MC__STOP;
    LIGHTOUT &= ~(LIGHT1 + LIGHT2);
  }
  else if (level == 255)
  {
//    TA1CTL = MC__STOP;
    LIGHTOUT |= LIGHT1 + LIGHT2;
  }
  else
  {
    LIGHTSEL |= LIGHT1 + LIGHT2;
    LIGHTOUT &= ~(LIGHT1 + LIGHT2);
#if 0
    // setup PWM for light
    // LIGHT1 <-- TA1.0
    // LIGHT2 <-- TA1.1
    // configure TA1.1 for COM switch
    TA1CTL |= TASSEL_1 + ID_3 + MC_1;
    TA1CCTL1 = OUTMOD_7;
    TA1CCR0 = 4096;
    TA1CCR1 = level << 4;
#endif
  }
}

static struct ctimer motor_timer;
void motor_stop(void* ptr)
{
  TB0CTL = MC__STOP;
  MOTORSEL &= ~(MOTOR);
  MOTOROUT &= ~(MOTOR);
}

void motor_on(uint8_t level, clock_time_t length)
{
  if (level == 0)
  {
    TB0CTL = MC__STOP;
    MOTORSEL &= ~(MOTOR);
    MOTOROUT &= ~(MOTOR);
  }
  else
  {
    MOTORSEL |= MOTOR;
    TB0CTL |= TBSSEL_1 + MC_1;
    TB0CCTL1 = OUTMOD_7;
    TB0CCR0 = 255;
    TB0CCR1 = level;
    ctimer_set(&motor_timer, length, motor_stop, NULL);
  }
}