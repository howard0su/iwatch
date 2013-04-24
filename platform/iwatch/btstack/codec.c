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

static const uint16_t config[] =
{
  0, 0x17d, 0x15, 0xfd, //Power Management
  0x118, 0x0, 0x149, 0x0, 0x0, 0x0, 0x8, 0x1ff, 0x0, 0x0, 0x108, 0x1ff, // Audio Control
  0xFFFF, 0xFFFF, // skip 2
  0x12c, 0x2c, 0x2c, 0x2c, 0x2c, //Equalizer
  0xFFFF, // skip 1
  0x32, 0x0, // DAC Limiter
  0xFFFF, // skip 1
  0x0, 0x0, 0x0, 0x0, //Notch Filter
  0xffff, // skip 1
  0x38, 0xb, 0x32, 0x0, //ALC Control
  0x8, 0xc, 0x93, 0xe9, //PLL Control
  0x0, // BYP Control
  0xFFFF, 0xFFFF, 0xFFFF, // skip 3
  0x2, 0x2a, 0x0, 0x100, 0x0, 0x2, 0x1, 0x0, 0x40, 0x40, 0xb9, 0x40, 0x1, //Input Output Mixer
};

static int codec_write(uint8_t reg, uint16_t data)
{
  return I2C_write(reg << 1 | ((data >> 8) & 0x01), (uint8_t)(data & 0Xff));
}

void codec_init()
{
  I2C_addr(CODEC_ADDRESS);
  //reset codec ?
  codec_write(REG_RESET, 0);
  __delay_cycles(5000);

  for(uint8_t i = 1; i <= 0x38; i++)
  {
    if (config[i] == 0xffff)
      continue;

    //log_info("write to %x with %x\n", i, config[i]);

    if (codec_write(i, config[i]))
    {
      I2C_done();
      return;
    }
  }

  process_post(ui_process, EVENT_CODEC_STATUS, (void*)BIT0);
  printf("initialize codec sucess\n");
  I2C_done();
}