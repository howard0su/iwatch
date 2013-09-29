#include "contiki.h"
#include "cfs/cfs-coffee.h"
#include "dev/flash.h"

#include "system.h"

struct system_data
{
	uint8_t system_debug; // 0 - retail system, 1 - debug system
	uint8_t system_reset; // 0 - don't reset, 1 - factory reset
	uint8_t system_testing; // 0 - normal, 1 - factory testing
	uint8_t serial[6];
};

#if defined(__GNUC__)
__attribute__ ((section(".infod")))
#else
#pragma constseg = INFOD
#endif
static const struct system_data data = {
	1, 0, 1,
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
};
#ifndef __GNUC__
#pragma constseg = default
#endif

#define INFOD (uint16_t*)0x1800

CASSERT(sizeof(struct system_data) <= 128, system_config_less_than_infod);

void system_init()
{
	struct system_data new_data;
	new_data = data;
	// check if need factory reset
	if (data.system_reset)
	{
		cfs_coffee_format();
		new_data.system_reset = 0;

		///TODO: Write system id into SPI security area

		// write the data
		flash_setup();
		flash_clear(INFOD);
  		flash_writepage(INFOD, (uint16_t*)&new_data, 128);
  		flash_done();
	}
}

uint8_t system_testing()
{
	return data.system_testing;
}

void system_reset()
{
  // backup rtc
  PMMCTL0 = PMMPW | PMMSWBOR;
}

void system_rebootToNormal()
{
	struct system_data new_data;
	new_data = data;

	new_data.system_testing = 0;
	// write the data
	flash_setup();
	flash_clear(INFOD);
	flash_writepage(INFOD, (uint16_t*)&new_data, 128);
	flash_done();

        system_reset();
}

uint8_t system_retail()
{
	return !data.system_debug; // if this is a debug system
}

void system_ready()
{
	if (system_retail() && !(SFRRPCR & SYSNMI))
	{
		SFRRPCR |= (SYSRSTRE + SYSRSTUP + SYSNMI);
		SFRIE1 &= ~NMIIE;
	}
 }

void system_shutdown(int shipping)
{
    __delay_cycles(100000);
    
    __disable_interrupt();
    __no_operation();

  // get back reset pin to normal
  SFRRPCR &= ~(SYSRSTRE + SYSRSTUP + SYSNMI);
  SFRIE1 |= NMIIE;
  
  // stop clock
  UCSCTL8 &= ~SMCLKREQEN;
  UCSCTL6 |= SMCLKOFF;
  
  P11SEL &= ~BIT0; // no aclk output, so bluetooth should be suspend
  bluetooth_shutdown();
  
  //shutdown backlight or motor
  backlight_shutdown();
  
  // shutdown LCD
  memlcd_DriverShutdown();
  
  //shutdown mpu6050
  mpu6050_shutdown();
  
  // shutdown I2c
  I2C_shutdown();
  
  // shutdown clock
  clock_shutdown();

  __delay_cycles(100000);
 
  /* turn off the regulator */
  PMMCTL0_H = PMMPW_H;
  PMMCTL0_L = PMMREGOFF;
  
  __disable_interrupt();
  for(int i =0 ; i < 100; i++)
    __low_power_mode_4();
  __no_operation();
  __no_operation();

  system_reset();
}

 void system_getserial(uint8_t *buf)
 {
 	//TODO
 	memcpy(buf, data.serial, 6);
 }