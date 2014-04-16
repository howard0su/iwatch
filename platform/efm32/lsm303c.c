/******************** (C) COPYRIGHT 2013 STMicroelectronics ********************
*
* File Name          : lsm303c.c
* Authors            : MSH - C&I BU - Application Team
*		     : Matteo Dameno (matteo.dameno@st.com)
*		     : Denis Ciocca (denis.ciocca@st.com)
*		     : Both authors are willing to be considered the contact
*		     : and update points for the driver.
* Version            : V.1.0.2
* Date               : 2013/Sep/19
* Description        : LSM303C magnetometer and accelerometer driver
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************
*******************************************************************************/
#include <stdio.h>

#include "contiki.h"
#include "contiki-conf.h"
#include "platform-conf.h"
#include "i2c.h"
#include "lsm303c.h"
#include "window.h"
#include "sys/etimer.h"
#include "dev/watchdog.h"

//==================== Magnetometer ============================================================
struct {
	unsigned int cutoff_us;
	u8 value;
} lsm303c_mag_odr_table[] = {
		{  12, LSM303C_MAG_ODR80  },
		{  25, LSM303C_MAG_ODR40   },
		{  50, LSM303C_MAG_ODR20   },
		{  100, LSM303C_MAG_ODR10 },
		{ 200, LSM303C_MAG_ODR5 },
		{ 400, LSM303C_MAG_ODR2_5},
		{ 800, LSM303C_MAG_ODR1_25},
		{ 1600, LSM303C_MAG_ODR0_625},
};

static const struct lsm303c_mag_platform_data default_lsm303c_mag_pdata = {
	.poll_interval = 100,
	.min_interval = LSM303C_MAG_MIN_POLL_PERIOD_MS,
	.fs_range = LSM303C_MAG_FS_4G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
};

struct reg_rw {
	u8 address;
	u8 default_value;
	u8 resume_value;
};

struct reg_r {
	u8 address;
	u8 value;
};

static struct status_registers {
	struct reg_r who_am_i;
	struct reg_rw cntrl1;
	struct reg_rw cntrl2;
	struct reg_rw cntrl3;
	struct reg_rw cntrl4;
	struct reg_rw cntrl5;
} status_registers = {
	.who_am_i.address = REG_WHOAMI_ADDR,
	.who_am_i.value = WHOIAM_VALUE,
	.cntrl1.address = REG_CNTRL1_ADDR,
	.cntrl1.default_value = REG_DEF_CNTRL1,
	.cntrl2.address = REG_CNTRL2_ADDR,
	.cntrl2.default_value = REG_DEF_CNTRL2,
	.cntrl3.address = REG_CNTRL3_ADDR,
	.cntrl3.default_value = REG_DEF_CNTRL3,
	.cntrl4.address = REG_CNTRL4_ADDR,
	.cntrl4.default_value = REG_DEF_CNTRL4,
	.cntrl5.address = REG_CNTRL5_ADDR,
	.cntrl5.default_value = REG_DEF_CNTRL5,
};

//==================== Acclerometer ============================================================	
struct {
	unsigned int cutoff_ms;
	unsigned int mask;
} lsm303c_acc_odr_table[] = {
		{    2, ACC_ODR800 },
		{    3, ACC_ODR400  },
		{    5, ACC_ODR200  },
		{   10, ACC_ODR100  },
#if (!OUTPUT_ALWAYS_ANTI_ALIASED)
		{   20, ACC_ODR50   },
		{  100, ACC_ODR10   },
#endif
};

//static int int1_gpio = LSM303C_ACC_DEFAULT_INT1_GPIO;

static struct lsm303c_acc_platform_data default_lsm303c_acc_pdata = {
	.fs_range = LSM303C_ACC_FS_2G,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,
	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,
	.poll_interval = 100,
	.min_interval = LSM303C_ACC_MIN_POLL_PERIOD_MS,
	.gpio_int1 = LSM303C_ACC_DEFAULT_INT1_GPIO,
};

static int lsm303c_Init(void)
{
	
}

//==================== Magnetometer ============================================================
static int lsm303c_hw_init(void)
{
}


//==================== Acclerometer ============================================================	
static int lsm303c_acc_hw_init(void)
{
}


