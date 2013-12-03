#include "contiki.h"
#include <stdio.h>
#include "uart1.h"
#include "isr_compat.h"

#define DCO_SPEED F_CPU

// 4800 (unused)
#define BAUD_4800 0x01
#define BitTime_4800   (DCO_SPEED / 4800)
#define BitTime_5_4800 (BitTime_4800 / 2)

// 9600
#define BAUD_9600 0x02
#define BitTime_9600   (DCO_SPEED / 9600)
#define BitTime_5_9600 (BitTime_9600 / 2)

// 19200
#define BitTime_19200   (DCO_SPEED / 19200)
#define BitTime_5_19200 (BitTime_19200 / 2)

// 38400
#define BitTime_38400   (DCO_SPEED / 38400)
#define BitTime_5_38400 (BitTime_38400 / 2)
// 57600
#define BitTime_57600   (DCO_SPEED / 57600)
#define BitTime_5_57600 (BitTime_57600 / 2)
// 115200
#define BitTime_115200   (DCO_SPEED / 115200)
#define BitTime_5_115200 (BitTime_115200 / 2)

static uint16_t BitTime;
static uint16_t BitTime_5;

static int uartTxDaTA0;                                         // UART internal variable for TX

PROCESS_NAME(protocol_process);

void
uart_init(char rate)
{
  UARTOUT &= ~(UARTRXBIT + UARTTXBIT); 
  UARTSEL |= UARTTXBIT + UARTRXBIT;                             // Timer function for TXD/RXD pins
  UARTDIR |= UARTTXBIT;                                         // Set all pins but RXD to output
  UARTDIR &= ~UARTRXBIT;

  TA0CCTL0 = OUT;                                              // Set TXD Idle as Mark = '1'
  TA0CCTL1 = SCS + OUTMOD0 + CM1 + CAP + CCIE;                           // Sync, Neg Edge, Capture, Int
  TA0CTL = TASSEL_2 + MC_2 + TACLR;                            // SMCLK, start in continuous mode

  BitTime = BitTime_115200;
  BitTime_5 = BitTime_5_115200;
}

void uart_changerate(char rate)
{
    switch (rate)
    {
        case BAUD_9600:
            BitTime = BitTime_9600;
            BitTime_5 = BitTime_5_9600;
            break;
        case BAUD_19200:
            BitTime = BitTime_19200;
            BitTime_5 = BitTime_5_19200;
            break;
        case BAUD_38400:
            BitTime = BitTime_38400;
            BitTime_5 = BitTime_5_38400;
            break;
        case BAUD_57600:
            BitTime = BitTime_57600;
            BitTime_5 = BitTime_5_57600;
            break;
        case BAUD_115200:
            BitTime = BitTime_115200;
            BitTime_5 = BitTime_5_115200;
            break;
    }
}


void uart_sendByte(uint8_t byte)
{
  while (TA0CCTL0 & CCIE);                                    // Ensure last char got TX'd
  TA0CCR0 = TA0R;                                              // Current state of TA counter
  TA0CCR0 += BitTime;                                      // One bit time till first bit
  TA0CCTL0 = OUTMOD0 + CCIE;                                  // Set TXD on EQU0, Int
  uartTxDaTA0 = byte;                                         // Load global variable
  uartTxDaTA0 |= 0x100;                                       // Add mark stop bit to TXData
  uartTxDaTA0 <<= 1;                                          // Add space start bit
}

extern int protocol_recv(unsigned char dataByte);
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
      TA0CCR1 += BitTime;                              // Add Offset to CCRx
      if (TA0CCTL1 & CAP)                                 // Capture mode = start bit edge
      {
          TA0CCTL1 &= ~CAP;                               // Switch capture to compare mode
          TA0CCR1 += BitTime_5;                    // Point CCRx to middle of D0
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
              char RXByte = rxData & 0xFF;                      // Store in global variable
              rxBitCnt = RXBITCNTWITHSTOP;                // Re-load bit counter
              rxData = 0;
              if (protocol_recv(RXByte))
              {
                process_poll(&protocol_process);
                LPM4_EXIT;       // Clear LPM0 bits from 0(SR)
              }
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

    TA0CCR0 += BitTime;                                      // Add Offset to CCRx
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
