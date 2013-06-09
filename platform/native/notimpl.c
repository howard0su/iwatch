#include "contiki.h"
#include "rtc.h"
#include "battery.h"

void rtc_init(){}
extern void rtc_setdate(uint16_t year, uint8_t month, uint8_t day)
{
}
void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec)
{
}

void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
  if (hour) *hour = 17;
  if (min) *min = 24;
  if (sec) *sec = 47;
}
void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday)
{
  if (year) *year = 2013;
  if (month) *month = 5;
  if (day) *day = 29;
  if (weekday) *weekday = 3;
}

uint8_t rtc_getmaxday(uint16_t year, uint8_t month)
{
  if (month == 2)
    {
      if (year%4==0 && year%100==0 || year%400==0)
	return 29;
      else
	return 28;
    }
  if(month==8)
    {
      return 31;
    }
  
  if (month % 2 == 0)
    {
      return 30;
    }
  else
    {
      return 31;
    }
}

uint8_t rtc_getweekday(uint16_t year, uint8_t month, uint8_t day)
{
	  if( month == 1 || month == 2 )
  {
    month += 12;
    year--;
  }

  return 1 + (( day + 2*month + 3*(month+1)/5 + year + year/4 ) %7);
}

void rtc_enablechange(uint8_t changes)
{

}

void watchdog_stop() {}
void watchdog_start() {}

void I2C_Init() {}
void button_init() {}
void backlight_init() {}
void rtimer_arch_init(void) {}
void rtimer_arch_schedule(rtimer_clock_t t){}
uint8_t battery_level() {return 71;}
BATTERY_STATE battery_state() {return BATTERY_CHARGING;}
void backlight_on(uint8_t level) {}
void mpu6050_init() {}


uint8_t btconfig_process(uint8_t event, uint16_t lparam, void* rparam) {return 0;}
uint8_t watch_process(uint8_t event, uint16_t lparam, void* rparam) {return 0;}
uint8_t control_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
uint8_t selftest_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}

void flash_setup(void) {}
void flash_done(void) {}
void flash_write(uint16_t *addr, unsigned short word) {}
void flash_clear(uint16_t *addr) {}

void flash_writepage(uint16_t *addr, const uint16_t *data, uint8_t size) {}
void __disable_interrupt() {}


int dmp_get_pedometer_step_count(unsigned long *count)
{
	*count = 1435;
}
int dmp_get_pedometer_walk_time(unsigned long *time)
{
	*time = 1257;
}
