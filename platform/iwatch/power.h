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

#define MODULE_LCD BIT0
#define MODULE_BT  BIT1
#define MODULE_CODEC BIT2

void power_pin(uint8_t module);
void power_unpin(uint8_t module);

#endif