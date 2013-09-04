#include "contiki.h"
#include "cfs/cfs-coffee.h"
#include "dev/flash.h"

#include "system.h"

struct system_data
{
	uint8_t system_debug; // 0 - debug system, 1 - retail system
	uint8_t system_reset; // 0 - don't reset, 1 - factory reset
	uint8_t system_testing; // 0 - normal, 1 - factory testing
	uint8_t serial[6];
	uint8_t txpower[3]; // 0:GFSK, 1:EDR2, 2:EDR3, valid from 0 - 15, or 0xff to take default
};

#if defined(__GNUC__)
__attribute__ ((section(".infod")))
#else
#pragma constseg = INFOD
#endif
static const struct system_data data = {
	0, 0, 0,
	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
	0x0d, 0x0d, 0x0d, 
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

	for(;;);	 // let watchdog reboot
}

uint8_t system_retail()
{
	return !data.system_debug; // if this is a debug system
}

// i = 0:GFSK, 1:EDR2, 2:EDR3
uint8_t system_txpower(uint8_t i)
{
	return data.txpower[i];
}

void system_ready()
{
	if (system_retail() && !(SFRRPCR & SYSNMI))
	{
		SFRRPCR |= (SYSRSTRE + SYSRSTUP + SYSNMI);
		SFRIE1 &= ~NMIIE;
	}
 }

 void system_getserial(uint8_t *buf)
 {
 	//TODO
 	memcpy(buf, data.serial, 6);
 }