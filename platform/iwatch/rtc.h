#ifndef _RTC_H_
#define _RTC_H_

struct datetime{
  uint16_t year;
  uint8_t  month;
  uint8_t  day;

  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
};

extern void rtc_init();
extern void rtc_setdate(uint16_t year, uint8_t month, uint8_t day);
extern void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec);
extern void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec);
extern void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday);
extern uint8_t rtc_getmaxday(uint16_t year, uint8_t month);

#define SECOND_CHANGE 0x01
#define MINUTE_CHANGE 0x02
extern void rtc_enablechange(uint8_t changes);

#endif