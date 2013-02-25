#include "contiki.h"
#include "i2c.h"
#include "isr_compat.h"
#include "sys/rtimer.h"

static uint8_t *rxdata;
static uint8_t rxlen;
static uint8_t *txdata;
static uint8_t txlen;

#define BUSYWAIT_UNTIL(cond, max_time)                                  \
  do {                                                                  \
    rtimer_clock_t t0;                                                  \
    t0 = RTIMER_NOW();                                                  \
    while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + (max_time))) {  \
      __bis_SR_register(LPM0_bits + GIE);                               \
      __no_operation();                                                 \
    }                                                                   \
  } while(0)

enum { STATE_IDL, STATE_RUNNING, STATE_DONE, STATE_ERROR} state;

void I2C_Init()
{
  // initialize i2c UCB1
  UCB1CTL1 |= UCSWRST;

  UCB1CTL0 = UCMODE_3 + UCMST + UCSYNC; // master, I2c mode, LSB
  UCB1CTL1 = UCSSEL__SMCLK + UCSWRST; // SMCLK for now
  UCB1BR0 = 20; // 8MHZ / 20 = 400Khz
  UCB1BR1 = 0;

  //Configure ports.
  P3SEL |= BIT7;
  P5SEL |= BIT4;

  UCB1CTL1 &= ~UCSWRST;
}

void I2C_readbytes(unsigned char reg, unsigned char *data, uint8_t len)
{
  txdata = &reg;
  txlen = 1;

  rxdata = data;
  rxlen = len;

  state = STATE_RUNNING;
  UCB1IE |= UCTXIE + UCRXIE;                         // Enable TX interrupt

  while (UCB1CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB1CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition

  BUSYWAIT_UNTIL(state != STATE_RUNNING, RTIMER_SECOND / 100);

  UCB1IE &= ~(UCTXIE + UCRXIE);
  return;
}

void I2C_write(unsigned char reg, unsigned char write_word)
{
  uint8_t data[2];
  data[0] = reg;
  data[1] = write_word;
  txdata = data;
  txlen = 2;
  rxlen = 0;

  state = STATE_RUNNING;
  UCB1IE |= UCTXIE;                         // Enable TX interrupt

  while (UCB1CTL1 & UCTXSTP);             // Ensure stop condition got sent
  UCB1CTL1 |= UCTR + UCTXSTT;             // I2C TX, start condition

  BUSYWAIT_UNTIL(state != STATE_RUNNING, RTIMER_SECOND / 100);

  UCB1IE &= ~UCTXIE;
}

void  I2C_addr(unsigned char address)
{
  UCB1CTL1 |= UCSWRST;
  UCB1I2CSA = address;
  UCB1CTL1 &= ~UCSWRST;
}

ISR(USCI_B1, USCI_B1_ISR)
{
  switch(__even_in_range(UCB1IV,12))
  {
  case  0: break;                           // Vector  0: No interrupts
  case  2: break;                           // Vector  2: ALIFG
  case  4: break;                           // Vector  4: NACKIFG
  case  6:    		                    // Vector  6: STTIFG
    {
      break;
    }
  case  8:      // Vector  8: STPIFG
    {
      break;
    }
  case 10:                                  // Vector 10: RXIFG
    {
      rxlen--;
      if (rxlen)
      {
        if (rxlen == 1)
        {
          UCB1CTL1 |= UCTXSTP;
        }
      }
      else
      {
        UCB1IFG &= ~UCRXIFG;
        state = STATE_DONE;
        __bic_SR_register_on_exit(LPM4_bits); // Exit LPM0
      }

      // read data to release SCL
      *rxdata++ = UCB1RXBUF;
      break;
    }
  case 12:                                  // Vector 12: TXIFG
    if (txlen)                          // Check TX byte counter
    {
      UCB1TXBUF = *txdata++;               // Load TX buffer
      txlen--;                          // Decrement TX byte counter
    }
    else
    {
      UCB1IFG &= ~UCTXIFG;                  // Clear USCI_B0 TX int flag
      // done, give stop flag
      if (rxlen == 0) // only do TX
      {
        UCB1CTL1 |= UCTXSTP;                  // I2C stop condition
        state = STATE_DONE;
        __bic_SR_register_on_exit(LPM4_bits); // Exit LPM0
      }
      else
      {
        UCB1CTL1 &= ~UCTR;         			// I2C RX
        UCB1CTL1 |= UCTXSTT;         		// I2C start condition
        if (rxlen == 1)
        {
          // wait Start signal send out
          while(UCB1CTL1 & UCTXSTT);
          // then send NACK and stop
          UCB1CTL1 |= UCTXSTP;
        }
      }
    }
    break;
  default: break;
  }
}
