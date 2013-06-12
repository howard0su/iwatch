#include "msp430.h"
#include "contiki.h"
#include "isr_compat.h"
#include "rtc.h"
#include "window.h"

PROCESS(rtc_process, "RTC Driver");
PROCESS_NAME(system_process);

static struct datetime now;

void rtc_init()
{
  // Configure RTC_A
  RTCCTL01 |= RTCHOLD + RTCMODE;
  // RTC enable, HEX mode, RTC hold
  // enable RTC time event interrupt

  RTCYEAR = 2013;                         // Year = 0x2010
  RTCMON = 1;                             // Month = 0x04 = April
  RTCDAY = 1;                            // Day = 0x05 = 5th
  RTCDOW = rtc_getweekday(13, 5, 1);
  RTCHOUR = 22;                           // Hour = 0x10
  RTCMIN = 10;                            // Minute = 0x32
  RTCSEC = 0;                            // Seconds = 0x45

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
    process_post(ui_process, EVENT_TIME_CHANGED, &now);
  }
  PROCESS_END();
}

uint8_t rtc_getweekday(uint16_t year, uint8_t month, uint8_t day)
{
  if( month == 1 || month == 2 )
  {
    month += 12;
    year--;
  }

  return 1 + (( day + 2*month + 3*(month+1)/5 + year + year/4 ) %7);
}

void rtc_setdate(uint16_t year, uint8_t month, uint8_t day)
{
  uint8_t weekday;

  weekday = rtc_getweekday(year, month, day);
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

uint8_t rtc_getmaxday(uint16_t year, uint8_t month)
{
  if (month == 2)
  {
    if (year%4==0 && year%100==0 || year%400==0)
      return 29;
    else
      return 28;
  }
  if(month==8)
  {
    return 31;
  }

  if (month % 2 == 0)
  {
    return 30;
  }
  else
  {
    return 31;
  }
}

void rtc_setalarm()
{

}

void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec)
{
  BUSYWAIT_UNTIL((RTCCTL01&RTCRDY), CLOCK_SECOND/8);

  if (hour) *hour = RTCHOUR;
  if (min) *min = RTCMIN;
  if (sec) *sec = RTCSEC;
}

void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday)
{
  BUSYWAIT_UNTIL((RTCCTL01&RTCRDY), CLOCK_SECOND/8);

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

  if (changes & TENMSECOND_CHANGE)
  {
    RTCPS1CTL = RT1PSIE | RT1IP1;
  }
  else
  {
    RTCPS1CTL &= ~RT1PSIE;
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
    process_poll(&rtc_process);
    LPM4_EXIT;
    break;
  case 12: break;                         // Reserved
  case 14: break;                         // Reserved
  case 16: break;                         // Reserved
  default: break;
  }
  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
