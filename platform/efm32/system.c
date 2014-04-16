#include "contiki.h"
#include "cfs/cfs-coffee.h"
//#include "dev/flash.h"
//#include "rtc.h"
#include "dev/watchdog.h"
#include <stdio.h>
#include "system.h"

struct system_data
{
  	uint8_t system_debug; // 0 - retail system, 1 - debug system
  	uint8_t system_reset; // 0 - don't reset, 1 - factory reset
  	uint8_t system_testing; // 0 - normal, 1 - factory testing
  	uint8_t system_lock;
  	uint8_t serial[6];
};

#if defined(__GNUC__)
	__attribute__ ((section(".infod")))
#else
	#pragma constseg = INFOD
#endif
static const struct system_data init_data = 
{
  	1, 0, 1,1,
  	0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
};
#ifndef __GNUC__
	#pragma constseg = default
#endif

#define INFOD (uint16_t*)0x1800

CASSERT(sizeof(struct system_data) <= 128, system_config_less_than_infod);

void system_init()
{
	const struct system_data *data = (struct system_data *)INFOD;
	struct system_data new_data;
	new_data = *data;
#ifdef NOTYET	
  	for(int i = 0; i < 9; i++)
  	{
    		printf("%d ", ((uint8_t*)&init_data)[i]);
  	}
  	printf("\n");
#endif
  	// check if need factory reset
  	if (data->system_reset)
  	{
#ifdef NOTYET  		
    		cfs_coffee_format();
#endif    		
    		new_data.system_reset = 0;
    
    		///TODO: Write system id into SPI security area
#ifdef NOTYET    		
    		flash_setup();
    		flash_clear(INFOD);
    		flash_writepage(INFOD, (uint16_t*)&new_data, 128);
    		flash_done();
#endif
  	}
}

uint8_t system_testing()
{
  	const struct system_data *data = (struct system_data *)INFOD;
  	return data->system_testing;
}

void system_reset()
{  	
  	rtc_save();   
  	watchdog_reboot();
}

void system_rebootToNormal()
{
  	const struct system_data *data = (struct system_data *)INFOD;
  	struct system_data new_data;
  	new_data = *data;
  
  	new_data.system_testing = 0;
  	
#ifdef NOTYET
  	flash_setup();
  	flash_clear(INFOD);
  	flash_writepage(INFOD, (uint16_t*)&new_data, 128);
  	flash_done();
#endif
  	system_reset();
}

void system_restore()
{

#ifdef NOTYET
  	uint8_t *d = (uint8_t*)&globaldata;
  	d += sizeof(uint16_t);
  	CRCINIRES = 0xFFFF;
  	for(int i = 2; i < sizeof(globaldata); i++)
    	CRCDIRB_L = *d;

  	if (globaldata.checksum == CRCINIRES)
    		rtc_restore(); // rtc have to restore since it has hardware registers
  	else
    		ped_reset(); // ped have to reset since no local storage
#endif    		
}

uint8_t system_retail()
{
  	const struct system_data *data = (struct system_data *)INFOD;
  	return !data->system_debug; // if this is a debug system
}

void system_ready()
{
#ifdef NOTYET
  	if (system_retail() && !(SFRRPCR & SYSNMI))
  	{
    		SFRRPCR |= (SYSRSTRE + SYSRSTUP + SYSNMI);
    		SFRIE1 &= ~NMIIE;
  	}
#endif	
 
}

void system_shutdown(int shipping)
{
#ifdef NOTYET
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
#endif  
  	system_reset();
}

const char * system_getserial()
{
  	const struct system_data *data = (struct system_data *)INFOD;
  	//TODO
  	return data->serial;
}



uint8_t system_locked()
{
  	const struct system_data *data = (struct system_data *)INFOD;
  	return data->system_lock;
}

void system_unlock()
{
#ifdef NOTYET
  	const struct system_data *data = (struct system_data *)INFOD;
  	struct system_data new_data;
  	new_data = *data;
  
  	new_data.system_lock = 0;
  	// write the data
  	flash_setup();
  	flash_clear(INFOD);
  	flash_writepage(INFOD, (uint16_t*)&new_data, 128);
  	flash_done();
#endif  
  	system_reset(); 
}


void system_resetfactory()
{
#ifdef NOTYET
    	const struct system_data *data = (struct system_data *)INFOD;
  	struct system_data new_data;
  	new_data = *data;
  
  	new_data.system_lock = 1;
  	new_data.system_reset = 1;

  	// write the data
  	flash_setup();
  	flash_clear(INFOD);
  	flash_writepage(INFOD, (uint16_t*)&new_data, 128);
  	flash_done();	
#endif  	
	system_reset(); 
}