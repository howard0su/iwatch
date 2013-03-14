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

#define MPU6050_ADDR 0x69

#define MPU_INT_SEL P1SEL
#define MPU_INT_DIR P1DIR
#define MPU_INT_IFG P1IFG
#define MPU_INT_IES P1IES
#define MPU_INT_IE  P1IE
#define MPU_INT_BIT BIT6

PROCESS(mpu6050_process, "MPU6050 Driver");

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  (10)

static struct etimer timer;

static void tap_cb(unsigned char direction, unsigned char count)
{
  printf("tab detected dir=%d, count=%d\n", direction, count);
  etimer_restart(&timer);
}

static void android_orient_cb(unsigned char orientation)
{
  printf("orient changed %d\n", orientation);
  etimer_restart(&timer);
}

/* The sensors can be mounted onto the board in any orientation. The mounting
* matrix seen below tells the MPL how to rotate the raw data from thei
* driver(s).
* TODO: The following matrices refer to the configuration on an internal test
* board at Invensense. If needed, please modify the matrices to match the
* chip-to-body matrix for your particular set up.
*/
static signed char gyro_orientation[9] = {-1, 0, 0,
0,-1, 0,
0, 0, 1};

/* These next two functions converts the orientation matrix (see
* gyro_orientation) to a scalar representation for use by the DMP.
* NOTE: These functions are borrowed from Invensense's MPL.
*/
static inline unsigned short inv_row_2_scale(const signed char *row)
{
  unsigned short b;

  if (row[0] > 0)
    b = 0;
  else if (row[0] < 0)
    b = 4;
  else if (row[1] > 0)
    b = 1;
  else if (row[1] < 0)
    b = 5;
  else if (row[2] > 0)
    b = 2;
  else if (row[2] < 0)
    b = 6;
  else
    b = 7;      // error
  return b;
}

static inline unsigned short inv_orientation_matrix_to_scalar(
                                                              const signed char *mtx)
{
  unsigned short scalar;

  /*
  XYZ  010_001_000 Identity Matrix
  XZY  001_010_000
  YXZ  010_000_001
  YZX  000_010_001
  ZXY  001_000_010
  ZYX  000_001_010
  */

  scalar = inv_row_2_scale(mtx);
  scalar |= inv_row_2_scale(mtx + 3) << 3;
  scalar |= inv_row_2_scale(mtx + 6) << 6;


  return scalar;
}

void mpu6050_init()
{
  // initialize I2C bus
  I2C_addr(MPU6050_ADDR);

  int result;
  unsigned char accel_fsr;
  unsigned short gyro_rate, gyro_fsr;
  unsigned long timestamp;

  result = mpu_init(NULL);
  if (result)
  {
    process_post(ui_process, EVENT_MPU_STATUS, (void*)0);
    return;
  }

  // configure INT pin
  MPU_INT_SEL &= ~MPU_INT_BIT;  // = 0 - I/O
  MPU_INT_DIR &= ~MPU_INT_BIT;  // = 0 - Input
  MPU_INT_IFG &= ~MPU_INT_BIT;  // no IRQ pending
  MPU_INT_IES &= ~MPU_INT_BIT;  // IRQ on 0->1 transition
  MPU_INT_IE  |=  MPU_INT_BIT;  // enable IRQ for P1.6

  /* Get/set hardware configuration. Start gyro. */
  /* Wake up all sensors. */
  mpu_set_sensors(INV_XYZ_ACCEL); //INV_XYZ_GYRO
  /* Push both gyro and accel data into the FIFO. */
  mpu_configure_fifo(INV_XYZ_ACCEL); //INV_XYZ_GYRO
  mpu_set_sample_rate(DEFAULT_MPU_HZ);
  /* Read back configuration in case it was set improperly. */
  mpu_get_sample_rate(&gyro_rate);
  mpu_get_gyro_fsr(&gyro_fsr);
  mpu_get_accel_fsr(&accel_fsr);

  /* To initialize the DMP:
  * 1. Call dmp_load_motion_driver_firmware(). This pushes the DMP image in
  *    inv_mpu_dmp_motion_driver.h into the MPU memory.
  * 2. Push the gyro and accel orientation matrix to the DMP.
  * 3. Register gesture callbacks. Don't worry, these callbacks won't be
  *    executed unless the corresponding feature is enabled.
  * 4. Call dmp_enable_feature(mask) to enable different features.
  * 5. Call dmp_set_fifo_rate(freq) to select a DMP output rate.
  * 6. Call any feature-specific control functions.
  *
  * To enable the DMP, just call mpu_set_dmp_state(1). This function can
  * be called repeatedly to enable and disable the DMP at runtime.
  *
  * The following is a short summary of the features supported in the DMP
  * image provided in inv_mpu_dmp_motion_driver.c:
  * DMP_FEATURE_LP_QUAT: Generate a gyro-only quaternion on the DMP at
  * 200Hz. Integrating the gyro data at higher rates reduces numerical
  * errors (compared to integration on the MCU at a lower sampling rate).
  * DMP_FEATURE_6X_LP_QUAT: Generate a gyro/accel quaternion on the DMP at
  * 200Hz. Cannot be used in combination with DMP_FEATURE_LP_QUAT.
  * DMP_FEATURE_TAP: Detect taps along the X, Y, and Z axes.
  * DMP_FEATURE_ANDROID_ORIENT: Google's screen rotation algorithm. Triggers
  * an event at the four orientations where the screen should rotate.
  * DMP_FEATURE_GYRO_CAL: Calibrates the gyro data after eight seconds of
  * no motion.
  * DMP_FEATURE_SEND_RAW_ACCEL: Add raw accelerometer data to the FIFO.
  * DMP_FEATURE_SEND_RAW_GYRO: Add raw gyro data to the FIFO.
  * DMP_FEATURE_SEND_CAL_GYRO: Add calibrated gyro data to the FIFO. Cannot
  * be used in combination with DMP_FEATURE_SEND_RAW_GYRO.
  */
  dmp_load_motion_driver_firmware();
  dmp_set_orientation(
                      inv_orientation_matrix_to_scalar(gyro_orientation));
  dmp_register_tap_cb(tap_cb);
  dmp_register_android_orient_cb(android_orient_cb);
  uint8_t dmp_features = DMP_FEATURE_TAP | DMP_FEATURE_ANDROID_ORIENT;
  dmp_enable_feature(dmp_features);
  dmp_set_fifo_rate(DEFAULT_MPU_HZ);
  mpu_set_dmp_state(1);

  I2C_done();
  process_start(&mpu6050_process, NULL);
}

extern int i2c_write(unsigned char slave_addr, unsigned char reg_addr,
                     unsigned char length, unsigned char const *data)
{
  return I2C_writebytes(reg_addr, data, length);
}

int i2c_read(unsigned char slave_addr, unsigned char reg_addr, unsigned char length, unsigned char *data)
{
  return I2C_readbytes(reg_addr, data, length);
}
extern void delay_ms(unsigned long num_ms)
{
  rtimer_clock_t t0;
  t0 = RTIMER_NOW();
  while(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + num_ms * RTIMER_SECOND / 1000)) {
    __bis_SR_register(LPM0_bits + GIE);
    __no_operation();
  }
}
void get_ms(unsigned long *count)
{
  *count = RTIMER_NOW() * RTIMER_SECOND / 1000;
}
int msp430_reg_int_cb(void (*cb)(void))
{
  return 1;
}

int port1_pin6()
{
  process_poll(&mpu6050_process);
  return 1;
}

static enum {STATE_RUNNING, STATE_SLEEP} state = STATE_RUNNING;
PROCESS_THREAD(mpu6050_process, ev, data)
{
  PROCESS_BEGIN();
  etimer_set(&timer, CLOCK_SECOND * 10);
  process_post(ui_process, EVENT_MPU_STATUS, (void*)BIT0);
  while(1)
  {
    PROCESS_WAIT_EVENT();
    // initialize I2C bus
    if (ev == PROCESS_EVENT_POLL)
    {
      static unsigned char more;

      if (state == STATE_SLEEP)
      {
        //mpu_lp_motion_interrupt(0, 0, 0);
        state = STATE_RUNNING;
      }

      /* This function gets new data from the FIFO when the DMP is in
      * use. The FIFO can contain any combination of gyro, accel,
      * quaternion, and gesture data. The sensors parameter tells the
      * caller which data fields were actually populated with new data.
      * For example, if sensors == (INV_XYZ_GYRO | INV_WXYZ_QUAT), then
      * the FIFO isn't being filled with accel data.
      * The driver parses the gesture data to determine if a gesture
      * event has occurred; on an event, the application will be notified
      * via a callback (assuming that a callback function was properly
      * registered). The more parameter is non-zero if there are
      * leftover packets in the FIFO.
      */
      I2C_addr(MPU6050_ADDR);
      do
      {
        short gyro[3], accel[3], sensors;
        unsigned long sensor_timestamp;
        long quat[4];

        dmp_read_fifo(gyro, accel, quat, &sensor_timestamp, &sensors,
                      &more);
        //printf("read one data\n");
        PROCESS_YIELD();
      }while(more);
      I2C_done();
    }
    else if (ev == PROCESS_EVENT_TIMER)
    {
      printf("enter sleep mode\n");
      state = STATE_SLEEP;
      watchdog_stop();
      //mpu_lp_motion_interrupt(500, 1, 5);
      watchdog_start();
    }
  }

  PROCESS_END();
}