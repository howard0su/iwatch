#include "contiki.h"
#include "i2c.h"
#include "sys/etimer.h"
#include "dev/watchdog.h"
#include <stdio.h>
#include "window.h"
#include "mpu3050_def.h"
#include "platform-conf.h"



//unsigned char TX_DATA[4];  	 
//unsigned char RX_BUF[10];       
char  test=0; 				 
short Gyro_X,Gyro_Y,Gyro_Z,Gyro_T;		 

PROCESS(mpu3050_process, "MPU3050 Driver");

static uint16_t read_interval;
static struct etimer timer;

static const GPIOMapping MPU3050_INT = {gpioPortF,4};


void delay_ms(unsigned long num_ms)
{
	uint16_t i = num_ms*1000;	
   	while(i) 
   	{ 
     		i--; 
   	}  
//  	BUSYWAIT_UNTIL(0, num_ms * RTIMER_SECOND / 1000);
}

void mpu3050Init(void)
{
//	process_start(&button_process, NULL);	

	//Config PF5 be input interrupt for MPU3050
	/* Configure button GPIO as input and configure output as high */
  	GPIO_PinModeSet(MPU3050_INT.port, MPU3050_INT.pin, gpioModeInput, 1);
  	/* Set falling edge interrupt for both ports. Enable interrupt */
	GPIO_IntConfig(MPU3050_INT.port, MPU3050_INT.pin, false, true, true);
	
	I2C_write(I2C0,MPU3050_Addr, MPU3050_PWR_MGM, H_RESET);
	delay_ms(100);
	I2C_write(I2C0,MPU3050_Addr, MPU3050_PWR_MGM, INTERNAL_OSC);
	I2C_write(I2C0,MPU3050_Addr, MPU3050_DLPF_FS_SYNC, LOW_PASS_FILTER | MPU3050_FS_SEL_2000DPS);
	I2C_write(I2C0,MPU3050_Addr, MPU3050_SMPLRT_DIV, SAMPLE_RATE_DIVISOR);	
	I2C_write(I2C0,MPU3050_Addr, MPU3050_INT_CFG, 0x00);
	
//	delay_ms(100);

//    	computeGyroRTBias();
		
}

void mpu3050_shutdown(void)
{
	GPIO_IntDisable(0x0010);	//disable GA_INT
	I2C_write(I2C0,MPU3050_Addr, MPU3050_PWR_MGM, MPU3050SLEEP);
	I2C_done();
}

void READ_MPU3050(void)
{
	uint8_t I2C2_Buffer_Rx[8];
	uint8_t i;
	
	
   	I2C_readbytes(I2C0, MPU3050_Addr,MPU3050_TEMP_OUT,I2C2_Buffer_Rx, 8); 
   	
   	for(i=0;i<8;i++)
   	{
   		printf("GX[%d]=%2x\n",i, I2C2_Buffer_Rx[i]);
   	}
/*
   	Gyro_T=	(I2C2_Buffer_Rx[0]<<8)|I2C2_Buffer_Rx[1];
   	Gyro_T = 35+ ((double) (Gyro_T + 13200)) / 280;	
	printf("GT=%2x\n", Gyro_T);		
   	
   	Gyro_X=	(I2C2_Buffer_Rx[2]<<8)|I2C2_Buffer_Rx[3];
   	Gyro_X/=16.4; 			
	printf("GX=%2x\n", Gyro_X);
	
   	Gyro_Y=	(I2C2_Buffer_Rx[4]<<8)|I2C2_Buffer_Rx[5];
   	Gyro_Y/=16.4; 			
	printf("GY=%2x\n", Gyro_Y);	

   	Gyro_Z=	(I2C2_Buffer_Rx[6]<<8)|I2C2_Buffer_Rx[7];
   	Gyro_Z/=16.4; 			
	printf("GY=%2x\n", Gyro_Z);	
*/

	   	
}

PROCESS_THREAD(mpu3050_process, ev, data)
{
  	PROCESS_BEGIN();
//  	_shakeCount = 0;
//  	_shaking = 0;

  	etimer_set(&timer, read_interval);
//  	process_post(ui_process, EVENT_MPU_STATUS, (void*)BIT0);
  	while(1)
  	{
    		PROCESS_WAIT_EVENT();
    		// initialize I2C bus
    		if (ev == PROCESS_EVENT_POLL || ev == PROCESS_EVENT_TIMER)
    		{
//        		I2C_addr(MPU6050_ADDR);
        		unsigned char more = 0;
        		do
        		{
        			READ_MPU3050();
//         			int accel[3];
//          			unsigned short last[3];
//          			unsigned char data[1020];
//          			unsigned short length = sizeof(data);
//          			int result = read_fifo_all(&length, data, &more);
#if 0
          			if (result == 0)
          			{
            				for (int index = 0; index < length; index += 6)
            				{
              					accel[0] = (((int)data[index + 0]) << 8) | data[index + 1];
              					accel[1] = (((int)data[index + 2]) << 8) | data[index + 3];
              					accel[2] = (((int)data[index + 4]) << 8) | data[index + 5];

              					if (read_interval == NORMAL_INTERVAL)
              					{
                					accel[0] >>= 6;
                					accel[1] >>= 6;
                					accel[2] >>= 6;

                					if (index > 0)
                					{
                  						if (!_shaking && CheckForShake(last, accel, ShakeThreshold) && _shakeCount >= 1)
                  						{
                    							//We are shaking
                    							_shaking = 1;
                    							_shakeCount = 0;
                    							process_post(ui_process, EVENT_KEY_PRESSED, (void*)KEY_TAP);
                  						}
                  						else if (CheckForShake(last, accel, ShakeThreshold))
                  						{
                    							_shakeCount++;
                  						}
                  						else if (!CheckForShake(last, accel, 50))
                  						{
                    							_shakeCount = 0;
                    							_shaking = 0;
                  						}

                					}
                					last[0] = accel[0];
					                last[1] = accel[1];
					                last[2] = accel[2];

					                if (ped_update_sample(accel) == 1)
					                {
					                  	ped_step_detect_run();
					                }
              					}
              					else
              					{
                					gesture_processdata(accel);
              					}
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
            				// reset fifo
            				I2C_write(MPU6050_RA_USER_CTRL, MPU6050_USERCTRL_FIFO_RESET_BIT | MPU6050_USERCTRL_FIFO_EN_BIT);
            				printf("fifo overflow\n");
            				more = 0;
            				break;
          			}
#endif
      			}while(more);
      			I2C_done();
      			etimer_set(&timer, read_interval);
    		}
  	}

  	PROCESS_END();
}
