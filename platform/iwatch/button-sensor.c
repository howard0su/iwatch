/* Dummy sensor routine */

#include "lib/sensors.h"
#include "isr_compat.h"
#include "button.h"

const struct sensors_sensor button_sensor;
static int status(int type);

struct _button_def
{
  unsigned char bitmask;
  struct timer debouncetimer;
}buttons[] = 
{
  {BIT0},{BIT1}, {BIT2}, {BIT6}
};

static int
value(int type)
{
  if (type >= 0 && type <4)
  {
    return (P2IN & buttons[type].bitmask) || !timer_expired(&buttons[type].debouncetimer);
  }
  else
  {
    return 0;
  }
}

static int
configure(int type, int c)
{
  int i;
  switch (type) {
  case SENSORS_HW_INIT:
    {
      for(i = 0; i < 4; i++)
      {
        // INPUT
        //P2DIR &= ~(buttons[i].bitmask);
        //P2SEL &= ~(buttons[i].bitmask);

        // ENABLE INT
        P2IES |= buttons[i].bitmask;
        P2IFG &= ~(buttons[i].bitmask);
        
        // pulldown in dev board
#warning change in release board
        P2REN |= buttons[i].bitmask;
        P2OUT &= ~(buttons[i].bitmask);
      }
      break;
    }
  case SENSORS_ACTIVE:
    if (c) {
      if(!status(SENSORS_ACTIVE)) {
        int s = splhigh();
        for(i = 0; i < 4; i++)
          P2IE |= buttons[i].bitmask;
        splx(s);
      }
    } else {
      for(i = 0; i < 4; i++)
        P2IE &= ~(buttons[i].bitmask);      
    }
    return 1;
  }
  return 0;
}

static int
status(int type)
{
  switch (type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return P2IE & buttons[1].bitmask;
  }
  return 0;
}

static int port2_button(int i)
{
  timer_set(&buttons[i].debouncetimer, CLOCK_SECOND/4);
  P2IFG &= ~(buttons[i].bitmask);
  sensors_changed(&button_sensor);
  
  return 1;
}

int port2_pin0()
{
  return port2_button(0);
}

int port2_pin1()
{
  return port2_button(1);
}

int port2_pin2()
{
  return port2_button(2);
}

int port2_pin6()
{
  return port2_button(3);
}

SENSORS_SENSOR(button_sensor, BUTTON_SENSOR,
	       value, configure, status);