#include "contiki.h"
#include "backlight.h"

#define LIGHTDIR P8DIR
#define LIGHTOUT P8OUT
#define LIGHTSEL P8OUT
#define LIGHT1   BIT6
#define LIGHT2   BIT5

#define MOTORDIR P4DIR
#define MOTOROUT P4OUT
#define MOTOR    BIT1

void backlight_init()
{
  LIGHTDIR |= LIGHT1 + LIGHT2;
  LIGHTOUT &= ~(LIGHT1 + LIGHT2);

  MOTORDIR |= MOTOR;
  MOTOROUT &= ~(MOTOR);
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

void motor_on(uint8_t level)
{
  if (level == 0)
  {
    MOTOROUT &= ~MOTOR;
  }
  else
  {
    MOTOROUT |= MOTOR;
  }
}