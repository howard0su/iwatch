#include "contiki.h"
#include "rtc.h"
#include "battery.h"
#include "ant/ant.h"
#include "dev/xmm.h"
uint8_t shutdown_mode;

void rtc_init(){}
extern void rtc_setdate(uint16_t year, uint8_t month, uint8_t day)
{
}
void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec)
{
}

void rtc_setalarm(uint8_t aday, uint8_t adow, uint8_t ahour, uint8_t aminute)
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
uint8_t control_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
uint8_t selftest_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
uint8_t upgrade_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}

void flash_setup(void) {}
void flash_done(void) {}
void flash_write(uint16_t *addr, unsigned short word) {}
void flash_clear(uint16_t *addr) {}

void flash_writepage(uint16_t *addr, const uint16_t *data, uint8_t size) {}
void __disable_interrupt() {}

unsigned long mpu_getsteptime()
{return 1257;}
unsigned long mpu_getsteps()
{return 1435;}
void mpu_gesturemode(uint8_t d)
{}

void ant_init(ModeEnum mode) {}
void ant_shutdown(void) {}

#ifdef _WINDOWS_H
#include <windows.h>
void nanosleep(int millisecond)
{
    Sleep(millisecond);
}
#endif

int spp_send(char* buf, int count)
{
  return 0;
}

void motor_on(uint8_t level, clock_time_t length)
{

}

//int spp_register_task(char* buf, int size, void (*callback)(char*, int))
//{}

void system_ready()
{
  printf("system is ready\n");
}

uint8_t system_testing()
{
  return 0;
}

uint8_t system_debug()
{
  return 0;
}

void system_rebootToNormal()
{

}

int mpu6050_selftest()
{
  return 0;
}

 void system_getserial(uint8_t *buf)
 {
  char fake[6] = {43,44,45,86,97,0xff};
  memcpy(buf, fake, 6);
 }


uint8_t
ANT_ChannelPower(
   uint8_t ucANTChannel_, 
   uint8_t ucPower_)
{
  return 0;
}


void hci_send_cmd_packet()
{
  
}

void SPI_FLASH_BufferRead_Raw(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
  xmem_pread(buf,   size,   offset);
}