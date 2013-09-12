#include "contiki.h"
#include <stdio.h>
#include "dev/uart1.h"
#include "isr_compat.h"

#define UARTOUT P1OUT
#define UARTSEL P1SEL
#define UARTDIR P1DIR
#define UARTIES P1IES
#define UARTIE  P1IE
#define UARTIFG P1IFG
#define UARTIN  P1IN

#define UARTTXBIT BIT1
#define UARTRXBIT BIT2

/**
* Baudrate
*/
#define BAUDRATE 		38400

/**
* Bit time
*/
#define UART1_TBIT_DIV_2        (F_CPU / BAUDRATE / 2)

/**
* Half bit time
*/
#define UART1_TBIT   (F_CPU / BAUDRATE)

static int uartTxDaTA0;                                         // UART internal variable for TX
static volatile char RXByte;                                           // Array to save rx'ed characters

PROCESS_NAME(protocol_process);

void
uart1_init(unsigned long ubr)
{
  #if 1
  UARTOUT &= ~(UARTRXBIT + UARTTXBIT); 
  UARTSEL |= UARTTXBIT + UARTRXBIT;                             // Timer function for TXD/RXD pins
  UARTDIR |= UARTTXBIT;                                         // Set all pins but RXD to output
  UARTDIR &= ~UARTRXBIT;

  TA0CCTL0 = OUT;                                              // Set TXD Idle as Mark = '1'
  TA0CCTL1 = SCS + OUTMOD0 + CM1 + CAP + CCIE;                           // Sync, Neg Edge, Capture, Int
  TA0CTL = TASSEL_2 + MC_2 + TACLR;                            // SMCLK, start in continuous mode

  #endif
}

uint8_t getByte()
{
  return RXByte;
}

void sendByte(uint8_t byte)
{
  while (TA0CCTL0 & CCIE);                                    // Ensure last char got TX'd
  TA0CCR0 = TA0R;                                              // Current state of TA counter
  TA0CCR0 += UART1_TBIT;                                      // One bit time till first bit
  TA0CCTL0 = OUTMOD0 + CCIE;                                  // Set TXD on EQU0, Int
  uartTxDaTA0 = byte;                                         // Load global variable
  uartTxDaTA0 |= 0x100;                                       // Add mark stop bit to TXData
  uartTxDaTA0 <<= 1;                                          // Add space start bit
}


/**
* ISR for TXD and RXD
*/
#define RXBITCNTWITHSTOP 9
ISR(TIMER0_A1, timer0_a1_interrupt)
{
  static int8_t rxBitCnt = RXBITCNTWITHSTOP;
  static int rxData = 0;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  switch (TA0IV)                      
  {
  case 2:                                      // TACCR1 CCIFG - UART RX
      TA0CCR1 += UART1_TBIT;                              // Add Offset to CCRx
      if (TA0CCTL1 & CAP)                                 // Capture mode = start bit edge
      {
          TA0CCTL1 &= ~CAP;                               // Switch capture to compare mode
          TA0CCR1 += UART1_TBIT_DIV_2;                    // Point CCRx to middle of D0
      }
      else 
      {
          rxData >>= 1;
          if (TA0CCTL1 & SCCI)                            // Get bit waiting in receive latch
          {  
              rxData |= 0x100;
          }
          rxBitCnt--;
          if (rxBitCnt == 0)                              // All bits RXed?
          {
              TA0CCTL1 |= CAP;                            // Switch compare to capture mode
              RXByte = rxData & 0xFF;                      // Store in global variable
              rxBitCnt = RXBITCNTWITHSTOP;                // Re-load bit counter
              rxData = 0;
              process_poll(&protocol_process);
              LPM4_EXIT;       // Clear LPM0 bits from 0(SR)
          }
      }
      break;

  default:
      break;
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}


//------------------------------------------------------------------------------
// Timer_A UART - Transmit Interrupt Handler
//------------------------------------------------------------------------------
//#pragma vector = TIMER1_A0_VECTOR
//__interrupt void Timer1_A0_ISR(void)
ISR(TIMER0_A0, Timer0_A0_ISR)
{
    static unsigned char txBitCnt = 10;
    ENERGEST_ON(ENERGEST_TYPE_IRQ);

    TA0CCR0 += UART1_TBIT;                                      // Add Offset to CCRx
    if (txBitCnt == 0)                                          // All bits TXed?
    {    
        TA0CCTL0 &= ~CCIE;                                      // All bits TXed, disable interrupt
        txBitCnt = 10;                                          // Re-load bit counter
    }
    else 
    {
        if (uartTxDaTA0 & 0x01) 
        {
          TA0CCTL0 &= ~OUTMOD2;                                 // TX Mark '1'
        }
        else 
        {
          TA0CCTL0 |= OUTMOD2;                                  // TX Space '0'
        }
        uartTxDaTA0 >>= 1;
        txBitCnt--;
    }
    ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
