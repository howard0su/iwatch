/****************************************************************
 *  Description: Header for power management
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
#ifndef _POWER_H
#define _POWER_H

#define POWER_SMCLK BIT0
#define POWER_ACLK  BIT1
#define POWER_HIGH  BIT2

extern void power_pin(int clock);
extern void power_unpin(int clock);
extern void power_sleep();

#endif