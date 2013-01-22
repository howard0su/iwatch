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

// 0 -> SMCLK, 1->ACLK, 3->HighPerf
static signed char PowerPinCounter[3];

void power_sleep()
{
  // uart1 && 
  if ((UCA3STAT & UCBUSY) || (UCA2STAT & UCBUSY))
  {
	LPM0;
  }
  else
  {
	LPM3;
  }
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
	PowerPinCounter[0]--;
	assert(PowerPinCounter[0] > 0);
  }
  if (clock & POWER_ACLK)
  {
	PowerPinCounter[1]--;
	assert(PowerPinCounter[1] > 0);
  }  
}