#define MOTION_DRIVER_TARGET_MSP430 1
#define MPU6050 1

#include "contiki.h"
#include "i2c.h"
#include "sys/etimer.h"
#include "dev/watchdog.h"
#include <stdio.h>
#include "window.h"

#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#include "pedometer/pedometer.h"

extern void gesture_processdata(int16_t *input);

#define MPU6050_ADDR 0x69

#define MPU_INT_SEL P1SEL
#define MPU_INT_DIR P1DIR
#define MPU_INT_IFG P1IFG
#define MPU_INT_IES P1IES
#define MPU_INT_IE  P1IE
#define MPU_INT_BIT BIT6

PROCESS(mpu6050_process, "MPU6050 Driver");

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  (50)

static struct etimer timer;

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
  int r = mpu_run_self_test(gyro, accel);
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
  // initialize I2C bus
  I2C_addr(MPU6050_ADDR);

  int result;

  printf("Initialize MPU6050...");
  result = mpu_init(NULL);
  if (result == -1)
  {
    goto error;
  }
  
  ped_step_detect_init();

  // configure INT pin
  MPU_INT_SEL &= ~MPU_INT_BIT;  // = 0 - I/O
  MPU_INT_DIR &= ~MPU_INT_BIT;  // = 0 - Input
  MPU_INT_IFG &= ~MPU_INT_BIT;  // no IRQ pending
  MPU_INT_IES &= ~MPU_INT_BIT;  // IRQ on 0->1 transition
  MPU_INT_IE  |=  MPU_INT_BIT;  // enable IRQ for P1.6

  if (mpu_set_sensors(INV_XYZ_ACCEL))
    goto error;
  
  if (mpu_set_accel_fsr(16))
    goto error;
  
//  if (mpu_lp_accel_mode(40))
//    goto error;
  
  if (mpu_configure_fifo(INV_XYZ_ACCEL))
    goto error;
    
  if (mpu_set_sample_rate(DEFAULT_MPU_HZ))
    goto error;

  I2C_done();
  printf("Done\n");

 // if (r)
  {
    process_start(&mpu6050_process, NULL);
    return;
  }

error:
  printf("Failed\n");
  process_post(ui_process, EVENT_MPU_STATUS, (void*)0);
  return;
}

void mpu6050_shutdown(void)
{
  MPU_INT_IE  &=  ~MPU_INT_BIT;  // enable IRQ for P1.6
  I2C_addr(MPU6050_ADDR);
  mpu_set_sensors(0);
  I2C_done();
}

int i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                     unsigned char length, unsigned char const *data)
{
#if 0
  if (length == 1)
    printf("write %d data=%x\n", reg_addr, *data);
  else
    printf("write %d len=%d\n", reg_addr, length);
#endif
  return I2C_writebytes(reg_addr, data, length);
}

int i2c_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data)
{
  //printf("read %d len=%d\n", reg_addr, length);
  return I2C_readbytes(reg_addr, data, length);
}

void delay_ms(unsigned long num_ms)
{
  BUSYWAIT_UNTIL(0, num_ms * RTIMER_SECOND / 1000);
}

void get_ms(unsigned long *count)
{
  if (count != NULL)
    *count = RTIMER_NOW() * RTIMER_SECOND / 1000;
}

int msp430_reg_int_cb(void (*cb)(void))
{
  return 1;
}

int port1_pin6()
{
  //process_poll(&mpu6050_process);
  return 0;
}

static enum {STATE_RUNNING, STATE_SLEEP} state = STATE_RUNNING;
PROCESS_THREAD(mpu6050_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_SECOND);
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
          short accel[3];
          char data[6];
          int result = mpu_read_fifo_stream(6, (char*)data, &more);
          if (result == 0)
          {
            accel[0] = (data[0] << 8) | data[1];
            accel[1] = (data[2] << 8) | data[3];
            accel[2] = (data[4] << 8) | data[5];
            ped_update_sample(accel);
            gesture_processdata(accel);

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
      etimer_reset(&timer);
    }
  }

  PROCESS_END();
}

void mpu_gesturemode(int d)
{
  
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