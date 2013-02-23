#include "contiki.h"
#include "i2c.h"
#include "sys/etimer.h"

#include <stdio.h>

#define SMPLRT_DIV  0x19        //�����ǲ����ʣ�����ֵ��0x07(125Hz)
#define CONFIG   0x1A           //��ͨ�˲�Ƶ�ʣ�����ֵ��0x06(5Hz)
#define GYRO_CONFIG  0x1B       //�������Լ켰������Χ������ֵ��0x18(���Լ죬2000deg/s)
#define ACCEL_CONFIG 0x1C       //���ټ��Լ졢������Χ����ͨ�˲�Ƶ�ʣ�����ֵ��0x01(���Լ죬2G��5Hz)
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
#define PWR_MGMT_1  0x6B //��Դ��������ֵ��0x00(��������)
#define WHO_AM_I   0x75 //IIC��ַ�Ĵ���(Ĭ����ֵ0x68��ֻ��)

#define MPU6050_ADDR 0x69
#include <in430.h>

PROCESS(mpu6050_process, "MPU6050 Driver");

void mpu6050_init()
{
  // initialize I2C bus
  I2C_Init();
  I2C_addr(MPU6050_ADDR);
  printf("Initialize MPU6050\n");
  I2C_write(PWR_MGMT_1, 0x00); //�������״̬
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