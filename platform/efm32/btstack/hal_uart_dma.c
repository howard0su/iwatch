/****************************************************************
*  Description: Implementation for UART for BTSTack
*    History:
*      Jun Su          2013/1/2        Created
*      Jun Su          2013/1/16       Move ISR to central place
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include <stdint.h>
#include "contiki.h"
#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "contiki-conf.h"
//#include "isr_compat.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "bluetooth.h"

#include "hal_compat.h"
#include <btstack/hal_uart_dma.h>
/*Define for test UART Tx Rx */

//#include "power.h"
static USART_TypeDef           * uart   = UART1;
static USART_InitAsync_TypeDef uartInit = USART_INITASYNC_DEFAULT;
// rx state
static uint16_t  bytes_to_read = 0;
static uint8_t * rx_buffer_ptr = 0;
static uint8_t   triggered;

static uint8_t   rx_temp_buffer;
static uint8_t   rx_temp_size = 0;

// handlers
static void (*rx_done_handler)(void) = NULL;
static void (*tx_done_handler)(void) = NULL;
static void (*cts_irq_handler)(void) = NULL;

/* DMA callback structure */
DMA_CB_TypeDef uart_dmaCallback;
/**
* @brief  Initializes the serial communications peripheral and GPIO ports
*         to communicate with the PAN BT .. assuming 16 Mhz CPU
*
* @param  none
*
* @return none
*/
/*
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
*/

void UART_TxTransferComplete(unsigned int channel, bool primary, void *user)
{
  	if (tx_done_handler)
    		(*tx_done_handler)();

  	if (triggered)
  	{
    		triggered = 0;
/*    		
    		LPM4_EXIT;
*/    		
  	}
}

void UART_DMA_CHN_Setup(void)
{	
	DMA_CfgChannel_TypeDef   txChnlCfg;
	DMA_CfgDescr_TypeDef     txDescrCfg;

  	/*** Setting up TX DMA ***/
  
	/* No callback needed for TX */
	uart_dmaCallback.cbFunc  = UART_TxTransferComplete;
	uart_dmaCallback.userPtr = NULL;
	
	/* Setting up channel */
	txChnlCfg.highPri   = false;
	txChnlCfg.enableInt = false;
	txChnlCfg.select    = DMAREQ_UART1_TXBL;
	txChnlCfg.cb        = &uart_dmaCallback;
	DMA_CfgChannel(DMA_CHN_UART_TX, &txChnlCfg);
	
	/* Setting up channel descriptor */
	txDescrCfg.dstInc  = dmaDataIncNone;
	txDescrCfg.srcInc  = dmaDataInc1;
	txDescrCfg.size    = dmaDataSize1;
	txDescrCfg.arbRate = dmaArbitrate1;
	txDescrCfg.hprot   = 0;
	DMA_CfgDescr(DMA_CHN_UART_TX, true, &txDescrCfg);
}

void hal_uart_dma_init(void)
{
	printf("UART init...\n");	
	//1. Config PB11 be BT_HCI_RTS input and interrupt enable. While BT cc2564 is ready to receive data from host, BT_HCI_RTS would be low
  	/* Configure button GPIO as input and configure output as high */
  	GPIO_PinModeSet(BT_RTS_PORT, BT_RTS_PIN, gpioModeInput, 1);
  	/* Set falling edge interrupt for both ports */
	GPIO_IntConfig(BT_RTS_PORT, BT_RTS_PIN, false, true, true);
  	
  	//2. Config PB12 be BT_HCI_CTS output. While host is ready to receive data from cc2564, it should be set low.
  	/* Configure button GPIO as input and configure output as high */
  	GPIO_PinModeSet(BT_CTS_PORT, BT_CTS_PIN, gpioModePushPull, 1);
	
	//3. Config PA1 be CMU_CLK1 output. it will output 32.768 KHZ clock
	/* Pin PA1 is configured to Push-pull */
	GPIO_PinModeSet(BT_CLK_PORT, BT_CLK_PIN, gpioModePushPull, 1);  	
	/* Select Clock Output 1 as Low Frequency Crystal Oscilator without prescalling (32 Khz) */
	CMU->CTRL = CMU->CTRL | CMU_CTRL_CLKOUTSEL1_LFXO ;
  	/* Route the Clock output pin to Location 0 and enable them */
	CMU->ROUTE = CMU_ROUTE_LOCATION_LOC0 | CMU_ROUTE_CLKOUT1PEN;  	  
		
  	//4. Config PB9 and PB10 be UART1. UART1_TX be PB9, UART1_RX be PB10  	
  	/* Pin PB9 is configured to Push-pull(U1_TX(BT_HCI_RX) ) */
  	/* To avoid false start, configure output U1_TX(BT_HCI_RX) as high on PB9 */  	
  	GPIO_PinModeSet(BT_RX_PORT, BT_RX_PIN, gpioModePushPull, 1);  	
  	/* Pin PB10 is configured to Input enabled */
  	GPIO_PinModeSet(BT_TX_PORT, BT_TX_PIN, gpioModeInput, 0);  	
  	
  	/* Enable clock for UART1 */
  	CMU_ClockEnable(cmuClock_UART1, true);
  	
  	//UART TX use DMA and RX use interrupt
  	hal_uart_dma_set_baud(115200);	

  	/* Prepare UART Rx and Tx interrupts */
  	USART_IntClear(uart, _UART_IF_MASK); 	  	  	
  	USART_IntEnable(uart, UART_IF_RXDATAV);
  	NVIC_ClearPendingIRQ(UART1_RX_IRQn);
  	NVIC_EnableIRQ(UART1_RX_IRQn);  
  	
  	NVIC_ClearPendingIRQ(UART1_TX_IRQn);
  	NVIC_EnableIRQ(UART1_TX_IRQn);	  
  	
  	/* Enable I/O pins at UART1 location #2 */
  	uart->ROUTE = UART_ROUTE_RXPEN | UART_ROUTE_TXPEN | UART_ROUTE_LOCATION_LOC2;

	UART_DMA_CHN_Setup();

	 /* Enable UART */
  	USART_Enable(uart, usartEnable);
}

/**

UART used in low-frequency mode
In this mode, the maximum USCI baud rate is one-third the UART source clock frequency BRCLK.

16000000 /  576000 = 277.77
16000000 /  115200 = 138.88
16000000 /  921600 =  17.36
16000000 / 1000000 =  16.00
16000000 / 2000000 =   8.00
16000000 / 2400000 =   6.66
16000000 / 3000000 =   3.33
16000000 / 4000000 =   2.00

*/
int hal_uart_dma_set_baud(uint32_t baud)
{
  	int result = 0;
  	uint32_t baudx=0;
  	
	USART_Reset(uart);  	
  	uartInit.enable       = usartDisable;   /* Don't enable UART upon intialization */
  	uartInit.refFreq      = 0;              /* Provide information on reference frequency. When set to 0, the reference frequency is */
  	uartInit.baudrate     = baud;         /* Baud rate */
  	uartInit.oversampling = usartOVS16;     /* Oversampling. Range is 4x, 6x, 8x or 16x */
  	uartInit.databits     = usartDatabits8; /* Number of data bits. Range is 4 to 10 */
  	uartInit.parity       = usartNoParity;  /* Parity mode */
  	uartInit.stopbits     = usartStopbits1; /* Number of stop bits. Range is 0 to 2 */
  	uartInit.mvdis        = false;          /* Disable majority voting */
  	uartInit.prsRxEnable  = false;          /* Enable USART Rx via Peripheral Reflex System */
  	uartInit.prsRxCh      = usartPrsRxCh0;  /* Select PRS channel if enabled */  	
    	
    	/* Initialize USART with uartInit struct */  	
  	USART_InitAsync(uart, &uartInit);  
  	  	
	baudx = USART_BaudrateGet(uart);
	printf("UART Baud Rate = [%x]\n", baudx);
  	return result;
}

void hal_uart_dma_set_block_received( void (*the_block_handler)(void))
{
  	rx_done_handler = the_block_handler;
}

void hal_uart_dma_set_block_sent( void (*the_block_handler)(void))
{	
  	tx_done_handler = the_block_handler;
}

void hal_uart_dma_set_csr_irq_handler( void (*the_irq_handler)(void))
{
  	if (the_irq_handler)
  	{
  		//Clear BT_RTS interrupt flag and enabel BT_RTS interrupt
  		GPIO_IntClear(1<<BT_RTS_PIN);	
  		GPIO_IntEnable(1<<BT_RTS_PIN);  		
  	}
  	else	
  	{
  		//disable BT_RTS interrupt
  		GPIO_IntDisable(1<<BT_RTS_PIN);  		
  	}

  	cts_irq_handler = the_irq_handler;
}

/**********************************************************************/
/**
* @brief  Disables the serial communications peripheral and clears the GPIO
*         settings used to communicate with the BT.
*
* @param  none
*
* @return none
**************************************************************************/
void hal_uart_dma_shutdown(void) 
{
	USART_Enable(uart, usartDisable);	//0 for disable UART_RX and UART_TX	
}

void hal_uart_dma_send_block(const uint8_t * data, uint16_t len)
{
  	//printf("hal_uart_dma_send_block, size %u\n\r", len); 
  	/* Wait until channel becomes available */
  	while(DMA_ChannelEnabled(DMA_CHN_UART_TX));  	

  	/* Activate DMA channel for TX */      
  	DMA_ActivateBasic(DMA_CHN_UART_TX,
                    	  true,
                    	  false,
                    	  (void *)&(uart->TXDATA),
                    	  (void *)data,
                    	  len - 1);  		
                    	  
}
// int used to indicate a request for more new data
void hal_uart_dma_receive_block(uint8_t *buffer, uint16_t len)
{
  	//printf("hal_uart_dma_receive_block, size %u temp_size: %u\n\r", len, rx_temp_size);
	/* disable RX interrupts */
	USART_IntDisable(uart, UART_IF_RXDATAV);

  	if (rx_temp_size)
  	{
    		*buffer = rx_temp_buffer;
    		rx_temp_size = 0;
    		buffer++;
    		len--;
  	}

  	if (len == 0)
  	{
    		if (rx_done_handler)
      			(*rx_done_handler)();
		USART_IntEnable(uart, UART_IF_RXDATAV);
    		//UCA0IE |= UCRXIE;    // enable RX interrupts
  	}
  	else
  	{
    		rx_buffer_ptr = buffer;
    		bytes_to_read = len;
    		USART_IntEnable(uart, UART_IF_RXDATAV);
    		//UCA0IE |= UCRXIE;    // enable RX interrupts

    		// enable send
    		USART_Enable(uart, usartEnableTx);
    		//BT_RTS_OUT &= ~BT_RTS_BIT;  // = 0 - RTS low -> ok
  	}
}

void hal_uart_dma_set_sleep(uint8_t sleep)
{
  	if (sleep)
  	{
  		USART_Enable(uart, usartDisable);
    		// wait for last byte sent out
/*    		
    		while(UCA0STAT & UCBUSY);
    		UCA0IE &= ~(UCRXIE | UCTXIE);
	    	UCA0CTL1 |= UCSWRST;                          //Reset State

    		power_unpin(MODULE_BT);
*/    		
  	}
  	else
  	{
  		USART_Enable(uart, usartDisable);
/*
    		UCA0IE |= UCRXIE | UCTXIE;
    		UCA0CTL1 &= ~UCSWRST;                          //Reset State
    		power_pin(MODULE_BT);
*/    		
  	}
  
  	triggered = 1;
}

void UART1_RX_IRQHandler(void)
{
	ENERGEST_ON(ENERGEST_TYPE_IRQ);
	if (uart->STATUS & UART_STATUS_RXDATAV)
	{
    		if (bytes_to_read == 0) 
    		{
    			GPIO_PinOutSet(BT_CTS_PORT, BT_CTS_PIN);
    			USART_IntDisable(uart, UART_IF_RXDATAV);
      			// put the data into buffer to avoid race condition
     			rx_temp_buffer = USART_Rx(uart);
      			rx_temp_size = 1;
      			return;
    		}
    		
    		*rx_buffer_ptr = USART_Rx(uart);
    		++rx_buffer_ptr;
    		--bytes_to_read;
    		if (bytes_to_read > 0) 
    		{
    			ENERGEST_OFF(ENERGEST_TYPE_IRQ);
    			return;
//      			break;
    		}
    		GPIO_PinOutSet(BT_CTS_PORT, BT_CTS_PIN);
    		USART_IntDisable(uart, UART_IF_RXDATAV);

    		if (rx_done_handler)
      			(*rx_done_handler)();
    
    		if (triggered)
    		{
      			triggered = 0;
//     			LPM4_EXIT;
    		}		
    		/* Clear RXDATAV interrupt */
    		USART_IntClear(uart, UART_IF_RXDATAV);		
	}		
	ENERGEST_OFF(ENERGEST_TYPE_IRQ);

}

// CTS ISR
int portB_pin11()
{
  	if (cts_irq_handler)
    		(*cts_irq_handler)();

  	if (triggered)
  	{
    		triggered = 0;
    		return 1;
  	}
  	else  
    		return 0;
}

void embedded_trigger(void)
{
  	process_poll(&bluetooth_process);
  	triggered = 1;
}
