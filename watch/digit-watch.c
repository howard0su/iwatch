#include "contiki.h"
#include "hal_lcd.h"

/*
 * This implement the digit watch
 * Wake up every 1 second and update the watch
 * If in 10 minutes, no key or other things
 * if get system key in non-suspend state, post event to system.
 */

static void drawClock(int day, int h, int m, int s)
{
  char buf[] = "00";
  halLcdBeginUpdate();

  halLcdClearScreen();

  halLcdVLine(72, 0, 9, 2, 0);
  halLcdVLine(72, 158, 167, 2, 0);
  halLcdHLine(0, 9, 84, 2, 0);
  halLcdHLine(134, 143, 84, 2, 0);

  buf[0] += day >> 4;
  buf[1] += day & 0x0f;
  halLcdHLine(93, 118, 72, 18, 0);
  halLcdPrintXY(buf, 98, 77, INVERT_TEXT);

  halLcdCircle(72, 84, 5, 1, 0);

  halLcdLine(72, 84, 40, 129, 1, 0);
  halLcdLine(72, 84, 87, 15, 3, 0);
  halLcdLine(72, 84, 112, 118, 3, 0);


  halLcdEndUpdate();
}

PROCESS(digitclock_process, "Digit Clock");

PROCESS_THREAD(digitclock_process, ev, data)
{
  PROCESS_BEGIN();
  static struct etimer et;
  etimer_set(&et, CLOCK_SECOND);

  drawClock(0x29, 0x4, 0x5, 0x37);

  PROCESS_END();
}