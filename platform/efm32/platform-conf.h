/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: platform-conf.h,v 1.1 2010/08/24 16:26:38 joxe Exp $
 */

/**
 * \file
 *         A brief description of what this file is
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef __PLATFORM_CONF_H__
#define __PLATFORM_CONF_H__
#include <stdint.h>
#include "efm32.h"
#include "efm32gg395f1024.h"
//#include "efm32gg990f1024.h"
//#include "em_cmu.h"
//#include "em_letimer.h"
#include <efm32.h>
#include <em_acmp.h>
#include <em_adc.h>
#include <em_assert.h>
#include <em_burtc.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_dma.h>
#include <em_ebi.h>
#include <em_emu.h>
#include <em_gpio.h>
#include <em_i2c.h>
#include <em_int.h>
#include <em_letimer.h>
#include <em_leuart.h>
#include <em_msc.h>
#include <em_rmu.h>
#include <em_rtc.h>
#include <em_timer.h>
#include <em_usart.h>
#include <em_wdog.h>
#include <bsp.h>
#include <bsp_bcp.h>
#include <bspconfig.h>
#include <bsp_trace.h>

//#include "efm32gg_cmu.h"

/*
 * Definitions below are dictated by the hardware and not really
 * changeable!
 */
#define TYNDALL 1

#define __no_operation() asm ("nop")	//define this to instead __no_operation() in msp430

#define dint() INT_Disable()
#define eint() INT_Enable()

/* SECTION: SYSTEM */
#define EFM32_SRAM_END 				(SRAM_BASE + SRAM_SIZE)
#define EFM32_BASE_PRI_DEFAULT 		(0x0UL << 5)
#define EFM32_IRQ_PRI_DEFAULT 		(0x4UL << 5)

/* CPU clock Freq dwfine*/
#define EFM32_USING_HFXO
#define EFM32_USING_LFXO

#if defined(EFM32_USING_HFXO)
 	#define EFM32_HFXO_FREQUENCY 		(48000000UL) 	                                               
#endif

#if defined(EFM32_USING_LFXO)
	#define EFM32_LETIMER_TOP_100HZ    (41)
#endif

typedef struct
{
  GPIO_Port_TypeDef   port;
  unsigned int        pin;
} GPIOMapping;

//=============== DMA Define ===============
//Define DMA Channel# for use

#define DMA_CHN_LCD_TX    	0
#define DMA_CHN_UART_TX     	1
#if 0
#define DMA_CHN_UART_RX     	2
#define DMA_CHN_SFLASH_TX	3
#define DMA_CHN_SFLASH_RX	4
#endif


void DMAInit(void);

//Definitions for UART ==================================================
/* Setup UART1 in async mode for RS232*/


#define NO_RX                    0
#define NO_TX                    NO_RX

#define BT_CLK_PORT	gpioPortA
#define BT_CLK_PIN	1
#define BT_SHUTD_PORT	gpioPortA
#define BT_SHUTD_PIN	3
#define BT_RX_PORT	gpioPortB
#define BT_RX_PIN	9
#define BT_TX_PORT	gpioPortB
#define BT_TX_PIN	10
#define BT_RTS_PORT	gpioPortB
#define BT_RTS_PIN	11
#define BT_CTS_PORT	gpioPortB
#define BT_CTS_PIN	12

__STATIC_INLINE void BT_STOP(void)
{
	 GPIO->P[BT_SHUTD_PORT].DOUTCLR = 1 << BT_SHUTD_PIN;
}

__STATIC_INLINE void BT_START(void)
{
	GPIO->P[BT_SHUTD_PORT].DOUTSET = 1 << BT_SHUTD_PIN;
}

#define BT_CLK_START()	CMU_CalibrateStart()
#define BT_CLK_STOP()	CMU_CalibrateStop()
//=============== Sensor define ================


// 
//void SPI_FLASH_Init(void);

/* Define termination character */
#define TERMINATION_CHAR    '.'

/* Declare a circular buffer structure to use for Rx and Tx queues */
#define BUFFERSIZE          512


/* Function prototypes */
void setupSWO(void);
int RETARGET_WriteChar(char c);
int RETARGET_ReadChar(void);
void uartSetup(void);
void cmuSetup(void);
void uartPutData(uint8_t * dataPtr, uint32_t dataLen);
uint32_t uartGetData(uint8_t * dataPtr, uint32_t dataLen);
void    uartPutChar(uint8_t charPtr);
uint8_t uartGetChar(void);
uint16_t __swap_bytes(uint16_t s);

#define HAVE_STDINT_H 1
//#define MSP430_MEMCPY_WORKAROUND 1
//#include "msp430def.h"


/* Types for clocks and uip_stats */
typedef unsigned short uip_stats_t;
typedef unsigned long clock_time_t;
//typedef unsigned long off_t;

/* the low-level radio driver */
//#define NETSTACK_CONF_RADIO   cc2420_driver

/*
 * Definitions below are dictated by the hardware and not really
 * changeable!
 */

/* DCO speed resynchronization for more robust UART, etc. */
/* Not needed from MSP430x5xx since it make use of the FLL */
//#define DCOSYNCH_CONF_ENABLED 0
//#define DCOSYNCH_CONF_PERIOD 30

//#define ROM_ERASE_UNIT_SIZE  512
#define XMEM_ERASE_UNIT_SIZE (64*1024L)

#define CFS_CONF_OFFSET_TYPE    long

/* Use the first 64k of external flash for node configuration */
#define NODE_ID_XMEM_OFFSET     (0 * XMEM_ERASE_UNIT_SIZE)

/* Use the second 64k of external flash for codeprop. */
#define EEPROMFS_ADDR_CODEPROP  (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_XMEM_CONF_OFFSET    (2 * XMEM_ERASE_UNIT_SIZE)
#define CFS_XMEM_CONF_SIZE      (1 * XMEM_ERASE_UNIT_SIZE)

#define CFS_RAM_CONF_SIZE 4096

#define ENERGEST_CONF_ON 0

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) {  \
      __no_operation();                                                 \
    }                                                                   \
  } while(0)

#ifndef CASSERT
#define CASSERT(exp, name) typedef int dummy##name [(exp) ? 1 : -1];
#endif

//#include "board.h"
#endif /* __PLATFORM_CONF_H__ */
