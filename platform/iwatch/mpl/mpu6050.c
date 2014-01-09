#define MOTION_DRIVER_TARGET_MSP430 1
#define MPU6050 1

#include "contiki.h"
#include "i2c.h"
#include "sys/etimer.h"
#include "dev/watchdog.h"
#include <stdio.h>
#include "window.h"

//#include "inv_mpu.h"
//#include "inv_mpu_dmp_motion_driver.h"

#include "pedometer/pedometer.h"

extern void gesture_processdata(int16_t *input);

#define MPU6050_ADDR 0x69

#define MPU_INT_SEL P1SEL
#define MPU_INT_DIR P1DIR
#define MPU_INT_IFG P1IFG
#define MPU_INT_IES P1IES
#define MPU_INT_IE  P1IE
#define MPU_INT_BIT BIT6

#define REG_SMPLRT_DIV 25
#define REG_INT_ENABLE 56
#define REG_INT_STATUS 58
#define REG_POWER_1 107
#define REG_FIFO_H 114
#define REG_FIFO_L 115
#define REG_FIFO_RW 116
#define REG_MOT_THR 31
#define REG_MOT_DETECT_CTRL 105
#define BIT_FIFO_OVERFLOW BIT4


const static uint8_t init_data[] =
{
  REG_POWER_1, 0x00,
  27, 0x18,
  28, 0x00,
  26, 0x03,
  26, 0x04,
  106, 0x20,
  55, 0x80,
  REG_POWER_1, 0x40,
  108, 0x3F,
  REG_POWER_1, 0,
  108, 0x07,
  28, 0x08,
  35, 0x00,
  106, 0x00,
  106, 0x04,
  106, 0x40,
  REG_INT_ENABLE, BIT6,
  35, 0x08,
  REG_SMPLRT_DIV, 0x13, // 1000/(0x19+1)  = 50HZ

  REG_MOT_THR, 20,
  REG_MOT_DETECT_CTRL, BIT4
};


PROCESS(mpu6050_process, "MPU6050 Driver");

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  (50)
#define GESTURE_MPU_HZ  (150)

static uint16_t read_interval = CLOCK_SECOND;
static struct etimer timer;

void delay_ms(unsigned long num_ms)
{
  BUSYWAIT_UNTIL(0, num_ms * RTIMER_SECOND / 1000);
}

static void tap_cb(unsigned char direction, unsigned char count)
{
  if (count == 1)
    process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_TAP);
  else
    process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_DOUBLETAP);
}

int mpu6050_selftest()
{
  long gyro[3], accel[3];
  int r = 0;// mpu_run_self_test(gyro, accel);
  printf("self test result %x\n", r);

  if (r != 0x03)
  {
    return -1;
  }
  
  printf("accel bias: %d, %d, %d\n", accel[0], accel[1], accel[2]);

  return 0;
}

void mpu6050_init()
{
    // configure INT pin
  MPU_INT_SEL &= ~MPU_INT_BIT;  // = 0 - I/O
  MPU_INT_DIR &= ~MPU_INT_BIT;  // = 0 - Input
  MPU_INT_IFG &= ~MPU_INT_BIT;  // no IRQ pending
  MPU_INT_IES &= ~MPU_INT_BIT;  // IRQ on 0->1 transition
  MPU_INT_IE  |=  MPU_INT_BIT;  // enable IRQ for P1.6

  // initialize I2C bus
  I2C_addr(MPU6050_ADDR);

#if 0
  int result;

  printf("Initialize MPU6050...");
  result = mpu_init(NULL);
  if (result == -1)
  {
    goto error;
  }
  
  ped_step_detect_init();

  printf("mpu_set_sensors\n");
  if (mpu_set_sensors(INV_XYZ_ACCEL))
    goto error;
  
  printf("mpu_set_accel_fsr\n");
  if (mpu_set_accel_fsr(4))
    goto error;
  
//  if (mpu_lp_accel_mode(40))
//    goto error;
  
  printf("mpu_configure_fifo\n");  
  if (mpu_configure_fifo(INV_XYZ_ACCEL))
    goto error;
    
  printf("mpu_set_sample_rate\n");  
  if (mpu_set_sample_rate(DEFAULT_MPU_HZ))
    goto error;


  printf("mpu6050_selftest\n");  
#else
  I2C_write(REG_POWER_1, 0x80);
  delay_ms(100);
  for(int i = 0; i < sizeof(init_data); i+=2)
  {
    I2C_writebytes(init_data[i], &init_data[i+1], 1);
  }
#endif

  I2C_done();
  printf("Done\n");
  process_start(&mpu6050_process, NULL);

  //if (mpu6050_selftest() == 0)
  {
    printf("\n$$OK MPU6050\n");
    return;
  }

error:
  printf("\n$$FAIL MPU6050\n");
  process_post(ui_process, EVENT_MPU_STATUS, (void*)0);
  return;
}

void mpu6050_shutdown(void)
{
  MPU_INT_IE  &=  ~MPU_INT_BIT;  // enable IRQ for P1.6
  I2C_addr(MPU6050_ADDR);
  I2C_write(REG_POWER_1, BIT6);
  I2C_done();
}

int port1_pin6()
{
  printf("motion deteceted");
  //process_poll(&mpu6050_process);
  return 0;
}

int read_fifo_all(unsigned short *length, unsigned char *data, unsigned char *more)
{
    unsigned char tmp[2];
    unsigned short fifo_count;

    
    if (I2C_readbytes(REG_FIFO_H, tmp, 2))
        return -1;
    fifo_count = (tmp[0] << 8) | tmp[1];
    if (fifo_count > (1024 >> 1)) {
        /* FIFO is 50% full, better check overflow bit. */
        if (I2C_readbytes(REG_INT_STATUS, tmp, 1))
            return -1;
        if (tmp[0] & BIT_FIFO_OVERFLOW) {
            return -2;
        }
    }

    if (*length >= fifo_count)
      *length = fifo_count;
    else
      *more = 1;
    
    //printf("there is %d\n", fifo_count);

    if (I2C_readbytes(REG_FIFO_RW, data, *length))
      return -1;
    
    return 0;
}


PROCESS_THREAD(mpu6050_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&timer, read_interval);
  process_post(ui_process, EVENT_MPU_STATUS, (void*)BIT0);
  while(1)
  {
    PROCESS_WAIT_EVENT();
    // initialize I2C bus
    if (ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_TIMER)
    {
        I2C_addr(MPU6050_ADDR);
        unsigned char more = 0;
        do
        {
          int16_t accel[3];
          char data[1020];
          unsigned short length = sizeof(data);
          int result = read_fifo_all(&length, (unsigned char*)data, &more);

          if (result == 0)
          {
            for (int index = 0; index < length; index += 6)
            {
              accel[0] = (data[index + 0] << 8) | data[index + 1];
              accel[1] = (data[index + 2] << 8) | data[index + 3];
              accel[2] = (data[index + 4] << 8) | data[index + 5];
              ped_update_sample(accel);
              gesture_processdata(accel);
            }
            continue;
          }
          else if (result == -1)
          {
            printf("Eror when read fifo\n");
            break;
          }
          else if (result == -2)
          {
            printf("fifo overflow\n");
            break;
          }

      }while(more);
      I2C_done();
      etimer_set(&timer, read_interval);
    }
  }

  PROCESS_END();
}

void mpu_gesturemode(int d)
{
  I2C_addr(MPU6050_ADDR);
  if (d)
  {
    I2C_write(REG_SMPLRT_DIV, (uint8_t)(1000/GESTURE_MPU_HZ - 1));
    read_interval = CLOCK_SECOND >> 3; // every 8/1 sec
  }
  else
  {
    I2C_write(REG_SMPLRT_DIV, (uint8_t)(1000/DEFAULT_MPU_HZ - 1));
    read_interval = CLOCK_SECOND; // every 3 second we read fifo buffer
  }
  I2C_done();
}

#if 0
#include "grlib/grlib.h"
#include "Template_Driver.h"
tContext context;
static void mpu_test()
{
  static int init = 0;
  static uint16_t saved_cnt = -1;
    
  if (!init)
  {
      memlcd_DriverInit();
      GrContextInit(&context, &g_memlcd_Driver);
      init = 1;
      GrContextForegroundSet(&context, ClrBlack);
      tRectangle rect = {0, 0, LCD_X_SIZE, LCD_Y_SIZE};
      GrRectFill(&context, &rect);
      GrContextForegroundSet(&context, ClrWhite);
  }
  
  if (step_cnt == saved_cnt)
  {
    return;
  }
  saved_cnt = step_cnt;
  
  GrContextFontSet(&context, &g_sFontNova28);
  char buf[32];
  sprintf(buf, "steps: %d   ", step_cnt); 
  GrStringDrawCentered(&context, buf, -1, LCD_X_SIZE/2, LCD_Y_SIZE/2, 1);

  GrFlush(&context);
}
#endif