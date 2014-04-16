#define	MPU3050_Addr   0xD0
// Registers
#define MPU3050_SMPLRT_DIV      0x15
#define MPU3050_DLPF_FS_SYNC    0x16
#define MPU3050_INT_CFG         0x17
#define MPU3050_TEMP_OUT	0x1B
#define MPU3050_TEMP_OUT_H      0x1B
#define MPU3050_TEMP_OUT_L      0x1C
#define	MPU3050_GYRO_XOUT_H	0x1D
#define	MPU3050_GYRO_XOUT_L	0x1E
#define	MPU3050_GYRO_YOUT_H	0x1F
#define	MPU3050_GYRO_YOUT_L	0x20
#define MPU3050_GYRO_ZOUT_H	0x21
#define MPU3050_GYRO_ZOUT_L	0x21
#define MPU3050_USER_CTRL       0x3D
#define MPU3050_PWR_MGM         0x3E

// Bits
#define MPU3050_FS_SEL_2000DPS  0x18
#define MPU3050_DLPF_10HZ       0x05
#define MPU3050_DLPF_20HZ       0x04
#define MPU3050_DLPF_42HZ       0x03
#define MPU3050_DLPF_98HZ       0x02
#define MPU3050_DLPF_188HZ      0x01
#define MPU3050_DLPF_256HZ      0x00

#define MPU3050_USER_RESET      0x01
#define MPU3050_CLK_SEL_PLL_GX  0x01

#define ACTL                  0x00
#define OPEN                  0x00
#define LATCH_INT_EN          0x20
#define INT_ANYRD_2CLEAR      0x10
#define RAW_RDY_EN            0x01
#define MPU3050SLEEP	      0x40	
#define H_RESET               0x80
#define INTERNAL_OSC          0x00

///////////////////////////////////////

#define LOW_PASS_FILTER 0x18    // 256 Hz Low pass filter, 8 kHz internal sample rate

#if (LOW_PASS_FILTER == 0x18)
#define SAMPLE_RATE_DIVISOR 0x07        // 1000 Hz = 8000/(7 + 1)
#else
#define SAMPLE_RATE_DIVISOR 0x00        // 1000 Hz = 1000/(0 + 1)
#endif