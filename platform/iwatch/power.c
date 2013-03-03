/****************************************************************
*  Description: Implementation for Power Management process
*    History:
*      Jun Su          2013/1/22        Created
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include "contiki.h"
#include "power.h"
#include "assert.h"

#include <stdio.h>

// 0 -> SMCLK, 1->ACLK, 3->HighPerf
static signed char PowerPinCounter[3];

void power_sleep()
{
  ENERGEST_OFF(ENERGEST_TYPE_CPU);

  if (PowerPinCounter[0]
      //|| (UCA0STAT & UCBUSY)        // BT uart
        //      || (UCA1STAT & UCBUSY)        // ANT uart
        //|| (UCB0STAT & UCBUSY)        // LCD
          //      || (UCA3STAT & UCBUSY)        // debug uart
          )
  {
    ENERGEST_ON(ENERGEST_TYPE_LPM0);
    __bis_SR_register(GIE | CPUOFF);
    ENERGEST_OFF(ENERGEST_TYPE_LPM0);
  }
  else
  {
    ENERGEST_ON(ENERGEST_TYPE_LPM3);
    __bis_SR_register(GIE | CPUOFF | SCG0 | SCG1);
    ENERGEST_OFF(ENERGEST_TYPE_LPM3);
  }
  ENERGEST_ON(ENERGEST_TYPE_CPU);
}

void power_pin(int clock)
{
  if (clock & POWER_SMCLK)
  {
	PowerPinCounter[0]++;
  }
  if (clock & POWER_ACLK)
  {
	PowerPinCounter[1]++;
  }
}

void power_unpin(int clock)
{
  if (clock & POWER_SMCLK)
  {
	assert(PowerPinCounter[0] > 0);
	PowerPinCounter[0]--;
  }
  if (clock & POWER_ACLK)
  {
	assert(PowerPinCounter[1] > 0);
	PowerPinCounter[1]--;
  }
}