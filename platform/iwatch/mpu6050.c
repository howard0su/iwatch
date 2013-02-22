#include "contiki.h"
#include "i2c.h"

#include <stdio.h>

#define SMPLRT_DIV  0x19        //陀螺仪采样率，典型值：0x07(125Hz)
#define CONFIG   0x1A           //低通滤波频率，典型值：0x06(5Hz)
#define GYRO_CONFIG  0x1B       //陀螺仪自检及测量范围，典型值：0x18(不自检，2000deg/s)
#define ACCEL_CONFIG 0x1C       //加速计自检、测量范围及高通滤波频率，典型值：0x01(不自检，2G，5Hz)
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

static unsigned int I2C_read16(unsigned char REG_Address)
{
  unsigned char H,L;
  H=I2C_read(REG_Address);
  L=I2C_read(REG_Address+1);
  return (H<<8)+L;   //合成数据
}

void mpu6050_init()
{
  // initialize I2C bus
  I2C_Init();
  I2C_addr(MPU6050_ADDR);
  printf("Initialize MPU6050\n");
  I2C_write(PWR_MGMT_1, 0x80); //reset
  waitAboutOneSecond();
  I2C_write(PWR_MGMT_1, 0x00); //解除休眠状态
  I2C_write(SMPLRT_DIV, 0x07);
  I2C_write(CONFIG, 0x06);
  I2C_write(GYRO_CONFIG, 0x18);
  I2C_write(ACCEL_CONFIG, 0x01);
  printf("Initialize finished\n");
  I2C_read(WHO_AM_I);
  uint16_t x = I2C_read16(ACCEL_XOUT_H);
  uint16_t y = I2C_read16(ACCEL_YOUT_H);
  uint16_t z = I2C_read16(ACCEL_ZOUT_H);
  printf("ACCEL X=%d, ACCEL Y=%d, ACCEL Z=%d\n", x, y, z);
}
