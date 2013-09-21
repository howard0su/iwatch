
#ifndef _STLV_CLIENT_H_
#define _STLV_CLIENT_H_

#include <stdint.h>

//-----------------------message sender------------------------
void send_echo(uint8_t* data, uint8_t size);

//return 0 if success
int begin_send_file(char* name);
int send_file_data(int fd, uint8_t* data, uint8_t size, void (*callback)(int), int para);
void end_send_file(int fd);

void send_file(char* name);
void send_sports_data(uint16_t* data, uint8_t size);
void send_sports_grid(uint8_t* data, uint8_t size);


//-----------------------send alarm conf------------------------
#define ALARM_MODE_NO_EXIST 0
#define ALARM_MODE_DISABLE  1
#define ALARM_MODE_ONCE     2
#define ALARM_MODE_HOURLY   3
#define ALARM_MODE_DAILY    4
#define ALARM_MODE_WEEKLY   5
#define ALARM_MODE_MONTHLY  6

typedef struct _alarm_conf_t
{
    uint8_t id;
    uint8_t mode;
    uint8_t day_of_month;
    uint8_t day_of_week;
    uint8_t hour;
    uint8_t minute;
}alarm_conf_t;

void send_alarm_conf(alarm_conf_t* data);
#endif

