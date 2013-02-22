#include "contiki.h"
#include "i2c.h"

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
 // UCB1IE |= UCTXIE + UCRXIE;                // Enable TX and RX interrupt
}

static void delay(void)
{
  unsigned int i,n;
  for(i=0;i<100;i++)
    for(n=0;n<0xff;n++);
}

unsigned char I2C_read(unsigned char reg)
{
  unsigned char RX_data;

  while(UCB1CTL1 & UCTXSTP);
  UCB1CTL1 |= UCTXSTT + UCTR;
  UCB1TXBUF = reg;
  while((UCB1IV & UCTXIFG) == 0);
  UCB1IV &= ~UCTXIFG;

  UCB1CTL1 &= ~UCTR;
  while(UCB1CTL1 & UCTXSTP);

  UCB1CTL1 |= UCTXSTT;
  while((UCB1CTL1 & UCTXSTT)==1);
  while((UCB1IV & UCRXIFG)==0);
  RX_data = UCB1RXBUF;

  delay();
  UCB1CTL1 |= UCTXSTP +UCTXNACK;
  while((UCB1CTL1 & UCTXSTP)==1);

  return RX_data;
}

void I2C_write(unsigned char reg, unsigned char write_word)
{
  while(UCB1CTL1 & UCTXSTP);
  UCB1CTL1 |= UCTXSTT + UCTR;
  UCB1TXBUF = reg;
  while((UCB1IFG & UCTXIFG) == 0);
  UCB1TXBUF = write_word;
  while((UCB1IFG & UCTXIFG) == 0);
  UCB1CTL1 |= UCTXSTP + UCTXNACK;
  while((UCB1CTL1 & UCTXSTP) == 1);
}

void  I2C_addr(unsigned char address)
{
  UCB1CTL1 |= UCSWRST;
  UCB1I2CSA = address;
  UCB1CTL1 &= ~UCSWRST;
}