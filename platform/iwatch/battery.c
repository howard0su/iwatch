#include "contiki.h"

#define BATINDIR P6DIR
#define BATININ  P6IN
#define BATINPIN BIT7

#define BATOUTDIR P7DIR
#define BATOUTOUT P7OUT
#define BATOUTPIN BIT5

void battery_init(void)
{
  // set input for BAT_IN
  BATINDIR &= ~BATINPIN;

  // set output for BAT_OUT
  BATOUTDIR |= BATOUTPIN;
  BATOUTOUT &= ~BATOUTPIN;
}
