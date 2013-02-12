/*
 * Copyright (c) 2010, Swedish Institute of Computer Science
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * @(#)$Id: uart1x.c,v 1.1 2010/08/24 16:23:20 joxe Exp $
 */

/*
 * Yet another machine dependent MSP430X UART1 code.
 * IF2, etc. can not be used here... need to abstract to some macros
 * later.
 */

#include "contiki.h"
#include <stdlib.h>

#include "sys/energest.h"
#include "dev/uart1.h"
#include "dev/watchdog.h"
#include "lib/ringbuf.h"
#include "dev/leds.h"
#include "isr_compat.h"

static int (*uart1_input_handler)(unsigned char c);

static volatile uint8_t transmitting;

/*---------------------------------------------------------------------------*/
void
uart1_set_input(int (*input)(unsigned char c))
{
  uart1_input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
uart1_writeb(unsigned char c)
{
  watchdog_periodic();
  /* Loop until the transmission buffer is available. */
  while((UCA3STAT & UCBUSY));

  /* Transmit the data. */
  UCA3TXBUF = c;
}
/*---------------------------------------------------------------------------*/
#if ! WITH_UIP /* If WITH_UIP is defined, putchar() is defined by the SLIP driver */
#endif /* ! WITH_UIP */
/*---------------------------------------------------------------------------*/
/**
 * Initalize the RS232 port.
 *
 */
void
uart1_init(unsigned long ubr)
{
  /* RS232 */
  UCA3CTL1 |= UCSWRST;            /* Hold peripheral in reset state */
  UCA3CTL1 |= UCSSEL_2;           /* CLK = SMCLK */

#if 0
  /* UCA1BR0 = 0x45;                 /\* 8MHz/115200 = 69 = 0x45 *\/ */
  UCA3BR0 = ubr & 0xff; //0x45; /* tested... */
  /* UCA1BR0 = 9; */
  UCA3BR1 = ubr >> 8;
  UCA3MCTL = UCBRS_3;             /* Modulation UCBRSx = 3 */
#endif
  UCA3BR0 = 69;  // from family user guide
  UCA3BR1 = 0;
  UCA3MCTL= 7 << 1;  // + 0.875

  P10SEL |= BIT4 + BIT5;                  /* P10.4,5 = USCI_A3 TXD/RXD */
  P10DIR &= ~BIT5;                 /* P10.5 = USCI_A3 RXD as input */
  P10DIR |= BIT4;                  /* P10.4 = USCI_A3 TXD as output */


  UCA3CTL1 &= ~UCSWRST;       /* Initialize USCI state machine */

  transmitting = 0;

  /* XXX Clear pending interrupts before enable */
  UCA3IE &= ~UCRXIFG;
  UCA3IE &= ~UCTXIFG;

  UCA3CTL1 &= ~UCSWRST;                   /* Initialize USCI state machine **before** enabling interrupts */
  UCA3IE |= UCRXIE;                        /* Enable UCA1 RX interrupt */
}
/*---------------------------------------------------------------------------*/
ISR(USCI_A3, uart1_rx_interrupt)
{
  uint8_t c;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);
  /*leds_toggle(LEDS_ALL);*/
  if(UCA3IV == 2) {
    if(UCA3STAT & UCRXERR) {
      c = UCA3RXBUF;   /* Clear error flags by forcing a dummy read. */
    } else {
      c = UCA3RXBUF;
      if(uart1_input_handler != NULL) {
        if(uart1_input_handler(c)) {
          LPM4_EXIT;
        }
      }
    }
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
