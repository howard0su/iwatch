#include "contiki.h"
#include "battery.h"
#include "isr_compat.h"
#include <stdio.h>

#define BATINDIR P6DIR
#define BATINSEL P6SEL
#define BATININ  P6IN
#define BATINPIN BIT7
#define BATLEVEL BIT4

#define BATOUTDIR P7DIR
#define BATOUTREN P7REN
#define BATOUTOUT P7OUT
#define BATOUTPIN BIT5

static uint8_t level = 70;

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

  BATINSEL |= BATLEVEL;

  ADC12CTL1 = ADC12SHP;                     // Use sampling timer
  ADC12IE = 0x01;                           // Enable interrupt
  ADC12MCTL0 = ADC12SREF_1 + 4;             // A4 as input
  ADC12CTL0 |= ADC12ENC;

  ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion

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

uint8_t battery_level(void)
{
  ADC12CTL0 = ADC12SHT02 + ADC12ON;         // Sampling time, ADC12 on

  ADC12CTL0 |= ADC12SC;                   // Start sampling/conversion
  printf("Battery Level: %d\n", (int)level);
  return level;
}

ISR(ADC12, _ADC12_ISR)
{
  switch(__even_in_range(ADC12IV,34))
  {
  case  0: break;                           // Vector  0:  No interrupt
  case  2: break;                           // Vector  2:  ADC overflow
  case  4: break;                           // Vector  4:  ADC timing overflow
  case  6:                                  // Vector  6:  ADC12IFG0
    level = ADC12MEM0 >> 4;
    ADC12CTL0 = 0;
    break;
  case  8: break;                           // Vector  8:  ADC12IFG1
  case 10: break;                           // Vector 10:  ADC12IFG2
  case 12: break;                           // Vector 12:  ADC12IFG3
  case 14: break;                           // Vector 14:  ADC12IFG4
  case 16: break;                           // Vector 16:  ADC12IFG5
  case 18: break;                                  // Vector 18:  ADC12IFG6
  case 20: break;                           // Vector 20:  ADC12IFG7
  case 22: break;                           // Vector 22:  ADC12IFG8
  case 24: break;                           // Vector 24:  ADC12IFG9
  case 26: break;                           // Vector 26:  ADC12IFG10
  case 28: break;                           // Vector 28:  ADC12IFG11
  case 30: break;                           // Vector 30:  ADC12IFG12
  case 32: break;                           // Vector 32:  ADC12IFG13
  case 34: break;                           // Vector 34:  ADC12IFG14
  default: break;
  }
}