#include "contiki.h"
#include "isr_compat.h"

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

#define MPU6050_ADDR 0xD0

static void I2C_Init()
{
  // initialize i2c UCB1
  UCB1CTL1 |= UCSWRST;
  
  UCB1CTL0 = UCMODE_3 + UCMST + UCSYNC; // master, I2c mode, LSB
  UCB1CTL1 = UCSSEL__SMCLK + UCSWRST; // SMCLK for now
  UCB1BR0 = 160; // 16MHZ / 160 = 100Khz
  UCB1BR1 = 0;
  
  UCB1I2CSA = MPU6050_ADDR >> 1;
  
  //Configure ports.
  P3SEL |= BIT7;
  P5SEL |= BIT4;
  
  UCB1CTL1 &= ~UCSWRST;
  UCB1IE |= UCTXIE + UCRXIE;                // Enable TX and RX interrupt
}

static unsigned char TxData[3];
static unsigned char TxDataLen;
static unsigned char RxData;
static unsigned char RxDataLen;
static unsigned char TxDataPtr;
static enum {NONE, RUNNING, DONE} State;

static void I2C_write(unsigned char address,unsigned char write_word)
{
  TxData[0] = address;
  TxData[1] = write_word;
  
  TxDataLen = 2;
  RxDataLen = 0;
  TxDataPtr = 0;
  
  State = RUNNING;
  UCB1CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
}

unsigned char I2C_read(unsigned char address)
{
  TxData[0] = address;
 
  TxDataLen = 1;
  RxDataLen = 1;
  TxDataPtr = 0;
  
  State = RUNNING;
  UCB1CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition
}

static unsigned int I2C_read16(unsigned char REG_Address)
{
  unsigned char H,L;
  H=I2C_read(REG_Address);
  L=I2C_read(REG_Address+1);
  return (H<<8)+L;   //合成数据
}

PROCESS(mpu6050_process, "MPU6050 process");

void mpu6050_init()
{
  // initialize I2C bus
  I2C_Init();
  
  process_start(&mpu6050_process, NULL);
}

PROCESS_THREAD(mpu6050_process, ev, data)
{
  PROCESS_BEGIN();
  
  I2C_write(PWR_MGMT_1, 0x00); //解除休眠状态
  PROCESS_WAIT_EVENT_UNTIL(State==DONE);
  I2C_write(SMPLRT_DIV, 0x07);
  PROCESS_WAIT_EVENT_UNTIL(State==DONE);
  I2C_write(CONFIG, 0x06);
  PROCESS_WAIT_EVENT_UNTIL(State==DONE);
  I2C_write(GYRO_CONFIG, 0x18);
  PROCESS_WAIT_EVENT_UNTIL(State==DONE);
  I2C_write(ACCEL_CONFIG, 0x01);
  PROCESS_WAIT_EVENT_UNTIL(State==DONE);
  
  printf("ACCEL X=%d, ACCEL Y=%d, ACCEL Z=%d\n",
		 I2C_read16(ACCEL_XOUT_H), 
		 I2C_read16(ACCEL_YOUT_H),
		 I2C_read16(ACCEL_ZOUT_H));
  
  PROCESS_END();
}

ISR(USCI_B1, USCI_B1_ISR)
{
  switch(__even_in_range(UCB1IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6: break;   		                // Vector  6: STTIFG
  case  8: break;                           // Vector  8: STPIFG
  case 10:                                  // Vector 10: RXIFG
    RxData = UCB1RXBUF;               // Move RX data to address PRxData
    process_poll(&mpu6050_process);
    State=DONE;
    LPM4_EXIT;
    break;
  case 12:                                  // Vector 12: TXIFG
    if (TxDataLen)                          // Check TX byte counter
    {
      UCB1TXBUF = TxData[TxDataPtr];        // Load TX buffer
      TxDataLen--;                          // Decrement TX byte counter
      TxDataPtr++;
    }
    else
    {
      if (RxDataLen)
      {
        UCB1CTL1 &= ~UCTR;         			// I2C RX
        UCB1CTL1 |= UCTXSTT;         		// I2C start condition
      }
      else
      {
        process_poll(&mpu6050_process);
        State=DONE;
        LPM4_EXIT;
      }
    }
  default: break;
  }
}


