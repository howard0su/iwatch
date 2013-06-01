#include <stdio.h>
#include "dev/uart1.h"

#define UARTOUT P1OUT
#define UARTDIR P1DIR
#define UARTBIT BIT1

int
putchar(int c)
{
  uart1_writeb((char)c);

  return c;
}
