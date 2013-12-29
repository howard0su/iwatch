#ifndef _MEMORY_H
#define _MEMORY_H
#include  <stdint.h>

extern union _data
{
  struct
  {
    uint8_t hour, minute, sec;
  }analog;

  struct
  {
    uint8_t hour0, minute;
  }digit;

  struct
  {
    uint8_t state;
    uint8_t times[3];
    uint32_t totaltime, lefttime;
  }countdown;

  struct
  {
    uint8_t state;
    uint8_t t[3];
  }config;

  struct
  {
    uint8_t state;
  }today;
}d;

#endif