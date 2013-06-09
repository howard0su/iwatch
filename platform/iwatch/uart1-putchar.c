#include <stdio.h>
#include "dev/uart1.h"
#include "isr_compat.h"

#define UARTOUT P1OUT
#define UARTSEL P1SEL
#define UARTDIR P1DIR
#define UARTBIT BIT1

/**
* Baudrate
*/
#define BAUDRATE 		38400

/**
* Bit time
*/
#define BIT_TIME        (F_CPU / BAUDRATE)

/**
* Half bit time
*/
#define HALF_BIT_TIME   (BIT_TIME / 2)


/**
* Bit count, used when transmitting byte
*/
static volatile uint8_t bitCount;

/**
* Value sent over UART when uart_putc() is called
*/
static volatile unsigned int TXByte;

void
uart1_init(unsigned long ubr)
{
  UARTSEL |= UARTBIT;
  UARTDIR |= UARTBIT;
}

int
putchar(int c)
{
  int x = splhigh();
  if ((x & GIE) == 0)
    return c;

  TXByte = c;

  TA0CCTL0 = OUT; 							// TXD Idle as Mark

  bitCount = 0xA; 						// Load Bit counter, 8 bits + ST/SP
  TA0CCR0 = TA0R; 							// Initialize compare register

  TA0CCR0 += BIT_TIME; 						// Set time till first bit
  TXByte |= 0x100; 						// Add stop bit to TXByte (which is logical 1)
  TXByte = TXByte << 1; 					// Add start bit (which is logical 0)

  TA0CCTL0 = CCIS_0 + OUTMOD_0 + CCIE + OUT; // Set signal, intial value, enable interrupts
  TA0CTL = TASSEL_2 + MC_2; 				// SMCLK, continuous mode
  splx(x);

  while ( TA0CCTL0 & CCIE ); 				// Wait for previous TX completion
  return c;
}

/**
* ISR for TXD and RXD
*/
ISR(TIMER0_A0, timer0_a0_interrupt)
{
  TA0CCR0 += BIT_TIME; 						// Add Offset to CCR0
  if ( bitCount == 0) { 					// If all bits TXed
    TA0CTL = TASSEL_2; 					// SMCLK, timer off (for power consumption)
    TA0CCTL0 &= ~ CCIE ; 					// Disable interrupt
  } else {
    if (TXByte & 0x01) {
      TA0CCTL0 = ((TA0CCTL0 & ~OUTMOD_7 ) | OUTMOD_1);  //OUTMOD_7 defines the 'window' of the field.
    } else {
      TA0CCTL0 = ((TA0CCTL0 & ~OUTMOD_7 ) | OUTMOD_5);  //OUTMOD_7 defines the 'window' of the field.
    }

    TXByte = TXByte >> 1;
    bitCount --;
  }
}
