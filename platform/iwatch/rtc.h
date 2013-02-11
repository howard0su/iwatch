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

void rtc_init();
void rtc_setdate(uint16_t year, uint8_t month, uint8_t day, uint8_t weekday);
void rtc_settime(uint8_t hour, uint8_t min, uint8_t sec);
void rtc_readtime(uint8_t *hour, uint8_t *min, uint8_t *sec);
void rtc_readdate(uint16_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday);

extern process_event_t timechangeevent;

#endif