#include "contiki.h"
#include "battery.h"

#define BATINDIR P6DIR
#define BATININ  P6IN
#define BATINPIN BIT7

#define BATOUTDIR P7DIR
#define BATOUTREN P7REN
#define BATOUTOUT P7OUT
#define BATOUTPIN BIT5

static void setoutputfloat()
{
  // set high impredence for BAT_OUT
  BATOUTDIR &= ~BATOUTPIN;
  BATOUTREN &= ~BATOUTPIN;  
}

static void setoutputhigh()
{
  BATOUTDIR |= BATOUTPIN;
  BATOUTOUT |= BATOUTPIN;
}

static void setoutputlow()
{
  BATOUTDIR |= BATOUTPIN;
  BATOUTOUT |= BATOUTPIN;
}

void battery_init(void)
{
  // set input for BAT_IN
  BATINDIR &= ~BATINPIN;

  setoutputfloat();
}


BATTERY_STATE battery_state(void)
{
  setoutputfloat();
  __delay_cycles(10);
  if (BATININ & BATINPIN) // if it is high
    return BATTERY_DISCHARGING;

  setoutputhigh();
  __delay_cycles(10);
  int instate = (BATININ & BATINPIN) == 1; // if it is high
  setoutputfloat();

  if (instate)
    return BATTERY_FULL;
  else
    return BATTERY_CHARGING;
}
