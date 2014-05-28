#include "contiki.h"
#include "bluetooth.h"

#include <stdio.h>

extern void __delay_cycles(unsigned long c);

void bluetooth_platform_init(void)
{
  BT_STOP();

  // wait clock stable
  __delay_cycles(1000);

  BT_START();
}


void bluetooth_platform_shutdown(void)
{
  printf("bluetooth process exit\n");
}