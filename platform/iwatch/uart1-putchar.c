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
static volatile int hasReceived;

PROCESS(input_process, "UART INPUT");

void
uart1_init(unsigned long ubr)
{
  #if 1
  UARTOUT &= ~(UARTRXBIT + UARTTXBIT); 
  UARTSEL |= UARTTXBIT + UARTRXBIT;                             // Timer function for TXD/RXD pins
  UARTDIR |= UARTTXBIT;                                         // Set all pins but RXD to output
  UARTDIR &= ~UARTRXBIT;

  TA0CCTL0 = OUT;                                              // Set TXD Idle as Mark = '1'
  TA0CCTL1 = SCS + CM1 + CAP + CCIE;                           // Sync, Neg Edge, Capture, Int
  TA0CTL = TASSEL_2 + MC_2;                                    // SMCLK, start in continuous mode

  process_start(&input_process, NULL);
  #endif
}

/*
 * protocol is like 1 byte command + 1 byte VERIFY
 *   7 6 5 4 3 2 1 0
 *   0 0 1 1 1 1 1 1 -> enter BSL
 *   0 0 1 1 1 0 1 0 -> enable debug log
 *
 *   0 0 0 x x x x x -> sync year 0 - 31
 *   0 0 1 0 x x x x -> sync month 0 - 12
 *   0 1 0 x x x x x -> sync day 0 - 31
 *   0 1 1 x x x x x -> sync hour 0 - 23
 *   1 0 x x x x x x -> sync minute 0 - 59
 *   1 1 x x x x x x -> sync second 0 - 59
 */
static void RunCommand(uint8_t payload)
{
  if (payload & 0x80)
  {
    if (payload & 0xC0)
    {
      // sync second
      //rtc_settime();
    }
    else
    {
      // set minute
    }
  }
  else
  {
    switch(payload & 0xE0)
    {
      case 0x00:
        break;
      case 0x20:
        break;
      case 0x40:
        break;
      case 0x60:
        break;
    }
  }

}
static enum
{
  WAITDATA,
  LEAD,
  PAYLOAD,
  CRC
}state;
static uint8_t payload;

PROCESS_THREAD(input_process, ev, data)
{
  PROCESS_BEGIN();
  state = WAITDATA;
  while(1)
  {
    PROCESS_WAIT_EVENT();
    if (!hasReceived)
      continue;
    hasReceived = 0;
    switch(state)
    {
      case WAITDATA:
        payload = RXByte;
        state = LEAD;
        break;
      case LEAD:
        if (RXByte == 0x5A)
        {
          state = PAYLOAD;
        }
        else
        {
          state = WAITDATA;
        }
        break;
      case PAYLOAD:
        payload = RXByte;
        state = CRC;
        break;
      case CRC:
        if (payload == ~RXByte)
        {
            RunCommand(payload);
        }
        state = WAITDATA;
        break;
    }
  }
  PROCESS_END();
}

int
putchar(int byte)
{
  //return 0;
  while (TA0CCTL0 & CCIE);                                    // Ensure last char got TX'd
  TA0CCR0 = TA0R;                                              // Current state of TA counter
  TA0CCR0 += UART1_TBIT;                                      // One bit time till first bit
  TA0CCTL0 = OUTMOD0 + CCIE;                                  // Set TXD on EQU0, Int
  uartTxDaTA0 = byte;                                         // Load global variable
  uartTxDaTA0 |= 0x100;                                       // Add mark stop bit to TXData
  uartTxDaTA0 <<= 1;                                          // Add space start bit

  return byte;
}


/**
* ISR for TXD and RXD
*/
ISR(TIMER0_A1, timer0_a1_interrupt)
{
  static unsigned char rxBitCnt = 8;
  static unsigned char rxData = 0;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  switch (TA0IV)                      
  {
  case 2:                                                 // TACCR1 CCIFG - UART RX
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
              rxData |= 0x80;
          }
          rxBitCnt--;
          if (rxBitCnt == 0)                              // All bits RXed?
          {
              RXByte = rxData; // Store in global variable
              hasReceived = 1;
              rxBitCnt = 8;                               // Re-load bit counter
              TA0CCTL1 |= CAP;                            // Switch compare to capture mode
              process_poll(&input_process);
              __bic_SR_register_on_exit(LPM4_bits);       // Clear LPM0 bits from 0(SR)
          }
      }
      break;
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}


//------------------------------------------------------------------------------
// Timer_A UART - Transmit Interrupt Handler
//------------------------------------------------------------------------------
//#pragma vector = TIMER1_A0_VECTOR
//__interrupt void Timer1_A0_ISR(void)
ISR(TIMER0_A0, Timer1_A0_ISR)
{
    static  char txBitCnt = 10;
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
