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
void setupSWOForPrint(void)
{
  	/* Enable GPIO clock. */
  	CMU->HFPERCLKEN0 |= CMU_HFPERCLKEN0_GPIO;

  	/* Enable Serial wire output pin */
  	GPIO->ROUTE |= GPIO_ROUTE_SWOPEN;

#if defined(_EFM32_GIANT_FAMILY) || defined(_EFM32_LEOPARD_FAMILY) || defined(_EFM32_WONDER_FAMILY)
  	/* Set location 0 */
  	GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) | GPIO_ROUTE_SWLOCATION_LOC0;

  	/* Enable output on pin - GPIO Port F, Pin 2 */
  	GPIO->P[5].MODEL &= ~(_GPIO_P_MODEL_MODE2_MASK);
  	GPIO->P[5].MODEL |= GPIO_P_MODEL_MODE2_PUSHPULL;
#else
  	/* Set location 1 */
  	GPIO->ROUTE = (GPIO->ROUTE & ~(_GPIO_ROUTE_SWLOCATION_MASK)) |GPIO_ROUTE_SWLOCATION_LOC1;
  	/* Enable output on pin */
  	GPIO->P[2].MODEH &= ~(_GPIO_P_MODEH_MODE15_MASK);
  	GPIO->P[2].MODEH |= GPIO_P_MODEH_MODE15_PUSHPULL;
#endif

  	/* Enable debug clock AUXHFRCO */
  	CMU->OSCENCMD = CMU_OSCENCMD_AUXHFRCOEN;

  	/* Wait until clock is ready */
  	while (!(CMU->STATUS & CMU_STATUS_AUXHFRCORDY));

  	/* Enable trace in core debug */
  	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  	ITM->LAR  = 0xC5ACCE55;
  	ITM->TER  = 0x0;
  	ITM->TCR  = 0x0;
  	TPI->SPPR = 2;
  	TPI->ACPR = 0xf;
  	ITM->TPR  = 0x0;
  	DWT->CTRL = 0x400003FE;
  	ITM->TCR  = 0x0001000D;
  	TPI->FFCR = 0x00000100;
  	ITM->TER  = 0x1;
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

  	setupSWOForPrint();

  	
  	printf("Hello EFM32\n");

        
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

/***************************************************************************//**
 * @brief
 *   Initialize the hardware drivers.
 *
 * @details
 *
 * @note
 *
 ******************************************************************************/
#if UNUSED
void rt_hw_driver_init(void)
{
#if 0
	/* Initialize DMA */
	rt_hw_dma_init();

    /* Select LFXO for specified module (and wait for it to stabilize) */
#if (!defined(EFM32_USING_LFXO) && defined(RT_USING_RTC))
#error "Low frequency clock source is needed for using RTC"
#endif

#if (!defined(EFM32_USING_LFXO )&& \
    (defined(RT_USING_LEUART0) || defined(RT_USING_LEUART1)))
#error "Low frequency clock source is needed for using LEUART"
#endif

	/* Initialize USART */
#if (defined(RT_USING_USART0) || defined(RT_USING_USART1) || \
    defined(RT_USING_USART2) || defined(RT_USING_UART0) || \
    defined(RT_USING_UART1))
	rt_hw_usart_init();
#endif

    /* Initialize LEUART */
#if (defined(RT_USING_LEUART0) || defined(RT_USING_LEUART1))
    rt_hw_leuart_init();
#endif

	/* Setup Console */
#if defined(EFM32_GXXX_DK)
    DVK_enablePeripheral(DVK_RS232A);
    DVK_enablePeripheral(DVK_SPI);
#elif defined(EFM32GG_DK3750)
 #if (RT_CONSOLE_DEVICE == EFM_UART1)
    DVK_enablePeripheral(DVK_RS232_UART);
 #elif (RT_CONSOLE_DEVICE == EFM_LEUART1)
    DVK_enablePeripheral(DVK_RS232_LEUART);
 #endif
#endif
	rt_console_set_device(CONSOLE_DEVICE);

	/* Initialize Timer */
#if (defined(RT_USING_TIMER0) || defined(RT_USING_TIMER1) || defined(RT_USING_TIMER2))
	rt_hw_timer_init();
#endif

	/* Initialize ADC */
#if defined(RT_USING_ADC0)
	rt_hw_adc_init();
#endif

	/* Initialize ACMP */
#if (defined(RT_USING_ACMP0) || defined(RT_USING_ACMP1))
	rt_hw_acmp_init();
#endif

	/* Initialize IIC */
#if (defined(RT_USING_IIC0) || defined(RT_USING_IIC1))
	rt_hw_iic_init();
#endif

	/* Initialize RTC */
#if defined(RT_USING_RTC)
	rt_hw_rtc_init();
#endif

    /* Enable SPI access to MicroSD card */
#if defined(EFM32_USING_SPISD)
 #if defined(EFM32_GXXX_DK)
    DVK_writeRegister(BC_SPI_CFG, 1);
 #elif defined(EFM32GG_DK3750)
    DVK_enablePeripheral(DVK_MICROSD);
 #endif
#endif

    /* Enable SPI access to Ethernet */
#if defined(EFM32_USING_ETHERNET)
 #if defined(EFM32GG_DK3750)
    DVK_enablePeripheral(DVK_ETH);
 #endif
#endif

    /* Initialize LCD */
#if defined(EFM32_USING_LCD)
    efm32_spiLcd_init();
#endif

    /* Initialize Keys */
#if defined(EFM32_USING_KEYS)
 #if defined(EFM32GG_DK3750)
    efm32_hw_keys_init();
 #endif
#endif
#endif	//#if 0
}
#endif
/***************************************************************************//**
 * @}
 ******************************************************************************/






