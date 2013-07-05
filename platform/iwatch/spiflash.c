#include "contiki.h"
#include <stdio.h>
#include "sys/rtimer.h"
#include "isr_compat.h"

#define CLKDIR P3DIR
#define CLKSEL P3SEL
#define CLKOUT P3OUT
#define CLKPIN BIT6

#define SPIDIR P5DIR
#define SPISEL P5SEL
#define SPIOUT P5OUT
#define SIPIN  BIT6
#define SOPIN  BIT7
#define CSPIN  BIT5

#define TXDONE 0x01
#define RXDONE 0x02

/*
* Transmits data on SPI connection
* - Busy waits until entire shift is complete
* - On devices with hardware SPI support, this function is identical to spi_exchange,
* with the execption of not returning a value
* - On devices with software (bit-bashed) SPI support, this function can run faster
* because it does not require data reception code
*/
static void usci_transmit( uint8_t data )
{
  UCA1TXBUF = data;
  while(( UCA1IFG & UCRXIFG ) == 0x00 ); // Wait for Rx completion (implies Tx is also complete)
  UCA1RXBUF;
}

/*
* Exchanges data on SPI connection
* - Busy waits until entire shift is complete
* - This function is safe to use to control hardware lines that rely on shifting being finalised
*/
static uint8_t usci_exchange( uint8_t data )
{
  UCA1TXBUF = data;
  while(( UCA1IFG & UCRXIFG ) == 0x00 ); // Wait for Rx completion (implies Tx is also complete)
  return( UCA1RXBUF );
}

static void spi_readwrite(uint8_t *rbuf, uint16_t rsize, const uint8_t *wbuf, uint16_t wsize)
{
  SPIOUT &= ~CSPIN; // pull CS low to enable chip 
  for(int i = 0; i < wsize; i++)
    usci_transmit(wbuf[i]);

  for(int i = 0; i < rsize; i++)
    rbuf[i] = usci_exchange(0);
  SPIOUT |= CSPIN; // pull CS high to disable chip 
}

void spiflash_readid(void)
{
	uint8_t id[8] = {0, 1, 2, 3, 4, 5, 6, 7};
	uint8_t command = 0x4b;
	spi_readwrite(id, 8, &command, 5);

	for(int i = 0; i < 8; i++)
		printf("%x ", id[i]);
	printf("\tr:%d w:%d\n", bytes_to_read, bytes_to_write);
}

void spiflash_read(void)
{

}

void spiflash_write(void)
{

}

void spiflash_init(void)
{
  // init SPI
  UCA1CTL1 = UCSWRST;

  UCA1CTL0 |= UCMST + UCSYNC + UCMSB + UCCKPH + UCCKPL; // master, 3-pin SPI mode, LSB
  UCA1CTL1 |= UCSSEL__SMCLK; // SMCLK for now
  UCA1BR0 = 8; // 16MHZ / 8 = 2Mhz
  UCA1BR1 = 0;
  UCA1MCTL = 0;

  //Configure ports.
  CLKDIR |= CLKPIN;
  CLKSEL |= CLKPIN;
  CLKOUT &= ~CLKPIN;

  SPIDIR |= SIPIN | CSPIN;
  SPIDIR &= ~SOPIN; // SO is input
  SPISEL |= SIPIN | SOPIN;
  SPIOUT &= ~SIPIN;
  SPIOUT |= CSPIN; // pull CS high to disable chip

  UCA1CTL1 &= ~UCSWRST;
  // read unique id

  // enable write
}