#include "msp430.h"
#include "contiki.h"
#include "isr_compat.h"
#include "rtc.h"
#include "window.h"

PROCESS(rtc_process, "RTC Driver");
PROCESS_NAME(system_process);
process_event_t timechangeevent;
static struct datetime now;

void rtc_init()
{
  timechangeevent = process_alloc_event();
  // Configure RTC_A
  RTCCTL01 |= RTCHOLD + RTCMODE;
  // RTC enable, BCD mode, RTC hold
  // enable RTC time event interrupt

  RTCYEAR = 0x2013;                         // Year = 0x2010
  RTCMON = 0x01;                             // Month = 0x04 = April
  RTCDAY = 0x01;                            // Day = 0x05 = 5th
  RTCDOW = 0x02;                            // Day of week = 0x01 = Monday
  RTCHOUR = 0x08;                           // Hour = 0x10
  RTCMIN = 0x10;                            // Minute = 0x32
  RTCSEC = 0x18;                            // Seconds = 0x45

  //  RTCADOWDAY = 0x2;                         // RTC Day of week alarm = 0x2
  //  RTCADAY = 0x20;                           // RTC Day Alarm = 0x20
  //  RTCAHOUR = 0x10;                          // RTC Hour Alarm
  //  RTCAMIN = 0x23;                           // RTC Minute Alarm

  process_start(&rtc_process, NULL);
  RTCCTL01 &= ~(RTCHOLD);                   // Start RTC calendar mode
}

PROCESS_THREAD(rtc_process, ev, data)
{
  PROCESS_BEGIN();

  while(1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
    process_post(ui_process, timechangeevent, &now);
  }
  PROCESS_END();
}

void rtc_setdate(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday)
{
  RTCYEAR = year;                         // Year = 0x2010
  RTCMON = month;                             // Month = 0x04 = April
  RTCDAY = day;                            // Day = 0x05 = 5th
  RTCDOW = weekday;                            // Day of week = 0x01 = Monday
}

void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec)
{
  RTCHOUR = hour;                           // Hour = 0x10
  RTCMIN = min;                            // Minute = 0x32
  RTCSEC = sec;                            // Seconds = 0x45
}

void rtc_setalarm()
{

}

void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
  if (hour) *hour = RTCHOUR;
  if (min) *min = RTCMIN;
  if (sec) *sec = RTCSEC;
}

void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday)
{
  if (year) *year = RTCYEAR;
  if (month) *month = RTCMON;
  if (day) *day = RTCDAY;
  if (weekday) *weekday = RTCDOW;
}

void rtc_enablechange(uint8_t changes)
{
  if (changes & MINUTE_CHANGE)
  {
    RTCCTL01 |= RTCTEV__MIN + RTCTEVIFG + RTCTEVIE;
  }
  else
  {
    RTCCTL01 &= ~(RTCTEV__MIN + RTCTEVIE);
  }

  if (changes & SECOND_CHANGE)
  {
    RTCCTL01 |= RTCRDYIE + RTCRDYIFG;
  }
  else
  {
    RTCCTL01 &= ~RTCRDYIE;
  }
}

ISR(RTC, RTC_ISR)
{
  ENERGEST_ON(ENERGEST_TYPE_IRQ);
  switch(__even_in_range(RTCIV,16))
  {
  case RTC_NONE:                          // No interrupts
    break;
  case RTC_RTCRDYIFG:                     // RTCRDYIFG
    {
      now.hour   = RTCHOUR;
      now.minute = RTCMIN;
      now.second = RTCSEC;
      now.year = RTCYEAR;
      now.month = RTCMON;
      now.day = RTCDAY;
      process_poll(&rtc_process);
      LPM4_EXIT;
      break;
    }
  case RTC_RTCTEVIFG:                     // RTCEVIFG
    {
      now.hour   = RTCHOUR;
      now.minute = RTCMIN;
      now.second = RTCSEC;
      now.year = RTCYEAR;
      now.month = RTCMON;
      now.day = RTCDAY;
      process_poll(&rtc_process);
      LPM4_EXIT;
      break;
    }
  case RTC_RTCAIFG:                       // RTCAIFG
    break;
  case RTC_RT0PSIFG:                      // RT0PSIFG
    break;
  case RTC_RT1PSIFG:                      // RT1PSIFG
    break;
  case 12: break;                         // Reserved
  case 14: break;                         // Reserved
  case 16: break;                         // Reserved
  default: break;
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
