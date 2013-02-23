#include "contiki.h"
#include "i2c.h"
#include "sys/etimer.h"

#include <stdio.h>

#define SMPLRT_DIV  0x19        //陀螺仪采样率，典型值：0x07(125Hz)
#define CONFIG   0x1A           //低通滤波频率，典型值：0x06(5Hz)
#define GYRO_CONFIG  0x1B       //陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define ACCEL_CONFIG 0x1C       //加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
#define INT_PIN_CFG 0x37
#define INT_ENABLE 0x38
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H  0x41
#define TEMP_OUT_L  0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define PWR_MGMT_1  0x6B //电源管理，典型值：0x00(正常启用)
#define WHO_AM_I   0x75 //IIC地址寄存器(默认数值0x68，只读)

#define MPU6050_ADDR 0x69
#include <in430.h>

PROCESS(mpu6050_process, "MPU6050 Driver");

void mpu6050_init()
{
  // initialize I2C bus
  I2C_Init();
  I2C_addr(MPU6050_ADDR);
  printf("Initialize MPU6050\n");
  I2C_write(PWR_MGMT_1, 0x00); //解除休眠状态
  I2C_write(SMPLRT_DIV, 0x07);
  I2C_write(CONFIG, 0x06);
  I2C_write(GYRO_CONFIG, 0x18);
  I2C_write(ACCEL_CONFIG, 0x01);

  // enable interrupt


  process_start(&mpu6050_process, NULL);
}

int port1_pin6()
{
  process_poll(&mpu6050_process);
  return 1;
}

PROCESS_THREAD(mpu6050_process, ev, data)
{
  static struct etimer timer;
  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_SECOND/5);
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (ev == PROCESS_EVENT_TIMER)
    {
      uint16_t x;
      I2C_read16(ACCEL_XOUT_H, &x);
      uint16_t y;
      I2C_read16(ACCEL_YOUT_H, &y);
      uint16_t z;
      I2C_read16(ACCEL_ZOUT_H, &z);
      printf("ACCEL X=%d, ACCEL Y=%d, ACCEL Z=%d\n", x, y, z);
      etimer_restart(&timer);
    }
  }
  PROCESS_END();
}