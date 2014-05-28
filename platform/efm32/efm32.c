/**************************************************************************//**
 * @file
 * @brief CPU and Board  controller init for EFM32GG395
 * @author Energy Micro AS
 * @version 3.20.3
 ******************************************************************************
 * @section License
 * <b>(C) Copyright 2012 Energy Micro AS, http://www.energymicro.com</b>
 *******************************************************************************
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 * 4. The source and compiled code may only be used on Energy Micro "EFM32"
 *    microcontrollers and "EFR4" radios.
 *
 * DISCLAIMER OF WARRANTY/LIMITATION OF REMEDIES: Energy Micro AS has no
 * obligation to support this Software. Energy Micro AS is providing the
 * Software "AS IS", with no express or implied warranties of any kind,
 * including, but not limited to, any implied warranties of merchantability
 * or fitness for any particular purpose or warranties against infringement
 * of any proprietary rights of a third party.
 *
 * Energy Micro AS will not be liable for any consequential, incidental, or
 * special damages, or any other relief, or for any claim by any third party,
 * arising from your use of this Software.
 *
 * This file is part of the Contiki operating system.
 *
 * @(#)$Id: efm32.c,v 0.1 2014/02/07 11:26:38 Eric Fan $
 *****************************************************************************/
/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "contiki.h"
#include "dev/watchdog.h"
#include "dev/leds.h"
#include "net/uip.h"
#include "em_dma.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define IS_NVIC_VECTTAB(VECTTAB) 		(((VECTTAB) == RAM_MEM_BASE) || \
						((VECTTAB) == FLASH_MEM_BASE))
#define IS_NVIC_OFFSET(OFFSET) 			((OFFSET) < 0x000FFFFF)



/***************************************************************************//**
 * @}
 ******************************************************************************/
/* DMA init structure */
DMA_Init_TypeDef dmaInit;

#define DMACTRL_CH_CNT      2
#define DMACTRL_ALIGNMENT   256

/** DMA control block array, requires proper alignment. */
#if defined (__ICCARM__)
#pragma data_alignment=DMACTRL_ALIGNMENT
DMA_DESCRIPTOR_TypeDef dmaControlBlock[DMACTRL_CH_CNT * 2];

#elif defined (__CC_ARM)
DMA_DESCRIPTOR_TypeDef dmaControlBlock[DMACTRL_CH_CNT * 2] __attribute__ ((aligned(DMACTRL_ALIGNMENT)));

#elif defined (__GNUC__)
DMA_DESCRIPTOR_TypeDef dmaControlBlock[DMACTRL_CH_CNT * 2] __attribute__ ((aligned(DMACTRL_ALIGNMENT)));

#else
#error Undefined toolkit, need to define alignment
#endif



extern void setup_SPI_DMA(void);
#define ITM_Port32(n) (*((volatile unsigned int *)(0xE0000000+4*n)))

/* Need to implement the  two Retarget IO functions with the read/write functions we want to use. */
/* This retargets printf to the SWO output. */
int RETARGET_WriteChar(char c)
{
  	return ITM_SendChar (c);
}

int RETARGET_ReadChar(void)
{
  	return 0;
}

uint16_t __swap_bytes(uint16_t s)
{
	return ((((s) & 0xFF00) >> 8)  | (((s) & 0x00FF) << 8));
}

/***************************************************************************//**
 * @brief
 *   Set the allocation and offset of the vector table
 *
 * @details
 *
 * @note
 *
 * @param[in] NVIC_VectTab
 *	 Indicate the vector table is allocated in RAM or ROM
 *
 * @param[in] Offset
 *   The vector table offset
 ******************************************************************************/
static void NVIC_SetVectorTable(
	uint32_t NVIC_VectTab,
	uint32_t Offset)
{
	/* Check the parameters */
	EM_ASSERT(IS_NVIC_VECTTAB(NVIC_VectTab));
	EM_ASSERT(IS_NVIC_OFFSET(Offset));

	SCB->VTOR = NVIC_VectTab | (Offset & (uint32_t)0x1FFFFF80);
}

/***************************************************************************//**
 * @brief
 *   Configure the address of vector table
 *
 * @details
 *
 * @note
 *
 ******************************************************************************/
static void NVIC_Configuration(void)
{
#ifdef  VECT_TAB_RAM
	/* Set the vector table allocated at 0x20000000 */
	NVIC_SetVectorTable(RAM_MEM_BASE, 0x0);
#else  /* VECT_TAB_FLASH  */
	/* Set the vector table allocated at 0x00000000 */
	NVIC_SetVectorTable(FLASH_MEM_BASE, 0x0);
#endif

	/* Set NVIC Preemption Priority Bits: 0 bit for pre-emption, 4 bits for
	   subpriority */
	NVIC_SetPriorityGrouping(0x7UL);

	/* Set Base Priority Mask Register */
	__set_BASEPRI(EFM32_BASE_PRI_DEFAULT);
}

void DMAInit(void)
{  
	CMU_ClockEnable(cmuClock_DMA, true);
	/* Initializing the DMA */
	dmaInit.hprot = 0;
	dmaInit.controlBlock = dmaControlBlock;
	DMA_Init(&dmaInit);
	
	setup_SPI_DMA();
}

/***************************************************************************//**
 * @brief
 *   Initialize the board.
 *
 * @details
 *
 * @note
 *
 ******************************************************************************/
void efm32_cpu_init(void)
{
        /* Initialize chip */
        CHIP_Init();
        
	/* If first word of user data page is non-zero, enable eA Profiler trace */        
}

void BT_CLK_Init(void)
{
	GPIO_PinModeSet(gpioPortA, 1, gpioModePushPull, 0);
	/* Select Clock Output 1 as Low Frequency Crystal Oscilator without prescalling (32 Khz) */
	CMU->CTRL = CMU->CTRL | CMU_CTRL_CLKOUTSEL1_LFXO ;
	/* Route the Clock output pin to Location 0 and enable them */
	CMU->ROUTE = CMU_ROUTE_LOCATION_LOC0 | CMU_ROUTE_CLKOUT1PEN;
	
	CMU_CalibrateCont(true);
	
}



