#include "contiki.h"
#include "rtc.h"
#include "battery.h"
#include "ant/ant.h"
#include <stdio.h>
#include <string.h>

#include "btstack/utils.h"

#define NOT_IMPL_VOID(f, ...) void f(__VAR_ARGS__){printf("[Not Impl]%s\n", __func__);}
#define NOT_IMPL(f, rettype, ret, ...) rettype f(__VAR_ARGS__){printf("[Not Impl]%s\n", __func__);return ret;}


uint8_t phone_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
uint8_t siri_process(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
uint8_t test_sleep(uint8_t ev, uint16_t lparam, void* rparam) {return 0;}
/*
void flash_setup(void) {}
void flash_done(void) {}
void flash_write(uint16_t *addr, unsigned short word) {}
void flash_clear(uint16_t *addr) {}
void flash_writepage(uint16_t *addr, const uint16_t *data, uint8_t size) {}
*/
void __disable_interrupt() {}

unsigned long mpu_getsteptime()
{return 1257;}
unsigned long mpu_getsteps()
{return 1435;}
void mpu_switchmode(int d)
{}

void ant_init(ModeEnum mode) {}
void ant_shutdown(void) {}

int mpu6050_selftest()
{
  return 0;
}

uint16_t ped_get_steps()
{
  return 6000;
}
void codec_setvolume(int a) {}
uint16_t ped_get_calorie() {return 10;}
uint16_t ped_get_time() {return 10;}
uint16_t ped_get_distance(){return 10;}

void ped_reset(){}

uint8_t
ANT_ChannelPower(
   uint8_t ucANTChannel_, 
   uint8_t ucPower_)
{
  return 0;
}


NOT_IMPL_VOID(WriteFirmware, void* data, uint32_t offset, int size);
NOT_IMPL_VOID(EraseFirmware);
NOT_IMPL_VOID(Upgrade);
NOT_IMPL(CheckUpgrade, int, 0);

NOT_IMPL_VOID(codec_wakeup);
NOT_IMPL_VOID(codec_suspend);

uint8_t codec_changevolume(int8_t diff)
{return 2;}
uint8_t codec_getvolume()
{return 2;}

