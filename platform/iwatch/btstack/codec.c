#include "contiki.h"
#include "window.h"

#include <stdint.h>
#include <stdio.h>
#include "i2c.h"
/*
 * Codec NAU1080 for BT
 */
#define CODEC_ADDRESS 0x1A

#define REG_RESET                       (0x00)
#define REG_POWER_MANAGEMENT1           (0x001)
#define REG_POWER_MANAGEMENT2	        (0x002)
#define REG_POWER_MANAGEMENT3           (0x003)
#define REG_AUDIO_INTERFACE	        (0x004)
#define REG_COMPANDING_CTRL             (0x005)
#define REG_CLK_GEN_CTRL	        (0x006)
#define REG_ADDITIONAL_CTRL	        (0x007)
#define REG_GPIO 	                (0x008)
#define REG_JACK_DETECT_CTRL1           (0x009)
#define REG_DAC_CTRL		        (0x00A)
#define REG_LEFT_DAC_DIGITAL_VOL        (0x00B)
#define REG_RIGHT_DAC_DIGITAL_VOL       (0x00C)
#define REG_JACK_DETECT_CTRL2  	        (0x00D)
#define REG_ADC_CTRL		  	(0x00E)
#define REG_LEFT_ADC_DIGITAL_VOL        (0x00F)
#define REG_RIGHT_ADC_DIGITAL_VOL       (0x010)
#define REG_EQ1_SHELF_LOW   	        (0x012)
#define REG_EQ2_PEAK1 			(0x013)
#define REG_EQ3_PEAK2 			(0x014)
#define REG_EQ4_PEAK3			(0x015)
#define REG_EQ5_HIGH_SHELF		(0x016)
#define REG_DAC_LIMITER1		(0x018)
#define REG_DAC_LIMITER2 		(0x019)
#define REG_NOTCH_FILTER1		(0x01B)
#define REG_NOTCH_FILTER2 		(0x01C)
#define REG_NOTCH_FILTER3 		(0x01D)
#define REG_NOTCH_FILTER4               (0x01E)
#define REG_ALC_CTRL1			(0x020)
#define REG_ALC_CTRL2 			(0x021)
#define REG_ALC_CTRL3 			(0x022)
#define REG_NOISE_GATE 	                (0x023)
#define REG_PLLN		        (0x024)
#define REG_PLL_K1		        (0x025)
#define REG_PLL_K2 		        (0x026)
#define REG_PLL_K3 		        (0x027)
#define REG_ATTENUATION_CTRL	        (0x028)
#define REG_3D_CONTROL                  (0x029)
#define REG_BEEP_CONTROL                (0x02B)
#define REG_INPUT_CTRL                  (0x02C)
#define REG_LEFT_INP_PGA_GAIN_CTRL      (0x02D)
#define REG_RIGHT_INP_PGA_GAIN_CTRL     (0x02E)
#define REG_LEFT_ADC_BOOST_CTRL         (0x02F)
#define REG_RIGHT_ADC_BOOST_CTRL 	(0x030)
#define REG_OUTPUT_CTRL			(0x031)
#define REG_LEFT_MIXER_CTRL		(0x032)
#define REG_RIGHT_MIXER_CTRL 		(0x033)
//LOUT1 --> HP-,ROUT1 --> HP+,LOUT2 --> SPKOUT-,ROUT2 --> SPKOUT+,OUT3 --> AUXOUT2,OUT4 --> AUXOUT1
#define REG_LOUT1_HP_VOLUME_CTRL	(0x034)
#define REG_ROUT1_HP_VOLUME_CTRL 	(0x035)
#define REG_LOUT2_SPKR_VOLUME_CTRL	(0x036)
#define REG_ROUT2_SPKR_VOLUME_CTRL 	(0x037)
#define REG_OUT3_MIXER_CTRL             (0x038)
#define REG_OUT4_MIXER_CTRL             (0x039)

extern uint8_t SMCLK_NEED;

static const uint16_t config[] =
{
  0, 0x17d, 0x15, 0x75, //Power Management
  0x090, 0x0, 0x1E0, 0x0, 0x0, 0x0, 0x8, 0x1ff, 0x0, 0xFF, 0x108, 0x1ff, // Audio Control
  0xFFFF, 0xFFFF, // skip 2
  0x12c, 0x2c, 0x2c, 0x2c, 0x2c, //Equalizer
  0xFFFF, // skip 1
  0x32, 0x00, // DAC Limiter
  0xFFFF, // skip 1
  0x0, 0x0, 0x0, 0x0, //Notch Filter
  0xffff, // skip 1
  0x38, 0xb, 0x32, 0x0, //ALC Control
  0x6, 0x9, 0x6E, 0x12F, //PLL Control for 16mhz MCLK
//  0x8, 0xc, 0x93, 0xE9, // PLL Control for 12Mhz MCLK
  0x0, // BYP Control
  0xFFFF, 0xFFFF, 0xFFFF, // skip 3
  0x3, 0x10, 0x0, 0x100, 0x0, 0x2, 0x1, 0x0, 0x40, 0x40, 0xbf, 0x40, 0x1, //Input Output Mixer
};

static int codec_write(uint8_t reg, uint16_t data)
{
  return I2C_write(reg << 1 | ((data >> 8) & 0x01), (uint8_t)(data & 0Xff));
}

static uint16_t codec_read(uint8_t reg)
{
  uint8_t data[2];
  I2C_readbytes(reg << 1, data, 2);
  
  return (data[0] << 8) | data[1];
}

#define AUDOUT P10OUT
#define AUDDIR P10DIR
#define AUDBIT BIT6

#define CLKSEL P11SEL
#define CLKDIR P11DIR
#define CLKBIT BIT2

#define PCODECDIR P7DIR
#define PCODECOUT P7OUT
#define PCODECBIT BIT7

void codec_shutdown()
{
  /*
  Mute DAC  DACMT[6] = 1
  Power Management  PWRM1 = 0x000
  Output stages
  MOUTEN[7]
  NSPKEN[6]
  PSPKEN[5]
  */
  printf("codec_shutdown\n");
  SMCLK_NEED--;
  CLKSEL &= ~CLKBIT;     // disable SMCLK

  I2C_addr(CODEC_ADDRESS, 1);
  codec_write(REG_POWER_MANAGEMENT1, 0);
  codec_write(REG_POWER_MANAGEMENT2, 0);
  codec_write(REG_POWER_MANAGEMENT3, 0);

  codec_write(REG_CLK_GEN_CTRL, 0x148);
  I2C_done();
}

uint8_t codec_changevolume(int8_t diff)
{
  uint8_t current;
  uint16_t value = config[REG_LOUT2_SPKR_VOLUME_CTRL];
  current = value & 0x3F;
  value &= ~0x3F;
  
  if (diff > 0)
  {
    if (current + diff <= 0x3f)
      current += diff;
  }
  else
  {
    if (current > -diff)
      current += diff;
  }

  value |= current;

  return current;
}

/* set volume, levle is from 0 - 255 */
void codec_setvolume(uint8_t level)
{
  uint16_t value = config[REG_LOUT2_SPKR_VOLUME_CTRL];
  value &= ~0x3F;
  value |= level >> 2;
}

void codec_wakeup()
{
  printf("codec_wakeup\n");
  CLKSEL |= CLKBIT;     // output SMCLK
  SMCLK_NEED++;

  I2C_addr(CODEC_ADDRESS, 1);
  codec_write(REG_POWER_MANAGEMENT1, config[REG_POWER_MANAGEMENT1]);
  codec_write(REG_POWER_MANAGEMENT2, config[REG_POWER_MANAGEMENT2]);
  codec_write(REG_POWER_MANAGEMENT3, config[REG_POWER_MANAGEMENT3]);

  codec_write(REG_CLK_GEN_CTRL, config[REG_CLK_GEN_CTRL]);
  I2C_done();
  printf("code_wakeup done\n");
}

void codec_init()
{
  AUDDIR |= AUDBIT;
  AUDOUT &= ~AUDBIT; // output direction, value = H

  CLKDIR |= CLKBIT;
  CLKSEL |= CLKBIT;     // output SMCLK

 PCODECDIR |= PCODECBIT;
 PCODECOUT |= PCODECBIT;

  SMCLK_NEED++;

  I2C_addr(CODEC_ADDRESS, 1);
  //reset codec ?
  codec_write(REG_RESET, 0);
  __delay_cycles(5000);


  for(uint8_t i = 1; i <= 0x38; i++)
  {
    if (config[i] == 0xffff)
      continue;
#if 0
    printf("write to %x with 0x%x ", i, config[i]);
    uint32_t d = config[i];
    for(int i = 0; i <= 8; i++)
    {
      if (d & 0x01)
        printf("BIT%d ", i);
      d = d >> 1;
    }
    printf("\n");
#endif
    if (codec_write(i, config[i]))
    {
      I2C_done();
      printf("initialize codec failed %d\n", i);
      return;
    }
  }

  process_post(ui_process, EVENT_CODEC_STATUS, (void*)BIT0);
  printf("initialize codec sucess\n");

#if 0
  for(int i = 1; i <= 0x38; i++)
  {
    uint32_t d = codec_read(i);
    printf("Codec reg: %x = 0x%lx ", i, d);
    for(int i = 0; i <= 8; i++)
    {
      if (d & 0x01)
        printf("BIT%d ", i);
      d = d >> 1;
    }
    printf("\n");
  }
#endif
  I2C_done();

  codec_shutdown();
}
