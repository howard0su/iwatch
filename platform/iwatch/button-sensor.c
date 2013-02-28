/* Dummy sensor routine */

#include "lib/sensors.h"
#include "isr_compat.h"
#include "button.h"
#include "window.h"

#define DEBOUNCE_PERIOD CLOCK_SECOND/4
#define LONGPRESS_PERIOD CLOCK_SECOND

const struct sensors_sensor button_sensor;
static int status(int type);

struct _button_def
{
  uint8_t bitmask;
  uint8_t released;
  struct timer debouncetimer;
  struct timer longpresstimer;
}buttons[] =
{
  {BIT0},{BIT1}, {BIT2}, {BIT6}
};

PROCESS(button_process, "Button Driver");

void button_init()
{
  process_start(&button_process, NULL);

  uint8_t x = splhigh();
  uint8_t i;
  for(i = 0; i < 4; i++)
  {
    // INPUT
    P2DIR &= ~(buttons[i].bitmask);
    P2SEL &= ~(buttons[i].bitmask);

    // ENABLE INT
    P2IES |= buttons[i].bitmask;

    // pullup in dev board
    P2REN |= buttons[i].bitmask;
    P2OUT |= buttons[i].bitmask;
    P2IFG &= ~(buttons[i].bitmask);

    P2IE |= buttons[i].bitmask;
  }

  splx(x);
}

static void inline poll_button()
{
  process_poll(&button_process);
}

PROCESS_THREAD(button_process, ev, data)
{
  PROCESS_BEGIN();
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_POLL)
    {
      uint8_t i;
      for(i = 0; i < 4; i++)
      {
        if (P2IFG & buttons[i].bitmask)
        {
          // key is released
          P2IFG &= ~buttons[i].bitmask;
          P2IE |= ~(buttons[i].bitmask);
          if (timer_expired(&buttons[i].longpresstimer))
          {
            process_post(ui_process, EVENT_KEY_LONGPRESSED, (void*)i);
          }
          else
          {
            process_post(ui_process, EVENT_KEY_PRESSED, (void*)i);
          }
        }
      }
    }
  }
  PROCESS_END();
}

static int port2_button(int i)
{
  if (P2IES & buttons[i].bitmask)
  {
    timer_set(&buttons[i].debouncetimer, DEBOUNCE_PERIOD);
    timer_set(&buttons[i].longpresstimer, LONGPRESS_PERIOD);
    P2IES ^= buttons[i].bitmask;
    P2IFG &= ~(buttons[i].bitmask);
  }
  else
  {
    P2IES ^= buttons[i].bitmask;
    if (timer_expired(&buttons[i].debouncetimer))
    {
      P2IE &= ~(buttons[i].bitmask);
      poll_button();
      return 1;
    }
    else
    {
      P2IFG &= ~buttons[i].bitmask;
    }
  }

  return 0;
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