
#ifndef _SPORTSDATE_H_
#define _SPORTSDATE_H_

#include <stdint.h>

//for running/biking store interval is 3 seconds
// the 2400 can hold 2 hours and about 46k in bytes
#define MAX_ROW_COUNT 2400

#define DATA_MODE_NORMAL    0x00
#define DATA_MODE_RUNNING   0x01
#define DATA_MODE_BIKING    0x02
#define DATA_MODE_TOOMSTONE 0xFF

typedef struct _record_desc_t
{
    uint8_t  mode;
    uint8_t  year;
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
    uint8_t  is_continue;
}record_desc_t;

void save_data_start(uint8_t mode, uint32_t timestamp);
void save_data(uint8_t mode, uint32_t timestamp, uint32_t data[], uint8_t size);
void save_data_end(uint8_t mode);

void save_activity(uint8_t mode, uint32_t data[], uint8_t size);

typedef struct _activity_row_t
{
    uint8_t signature;
    uint8_t mode;
    uint8_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    uint32_t data[6];

}activity_raw_t;
uint8_t load_history(activity_raw_t row[], uint8_t size);
void flush_activity_rows(activity_raw_t row[], uint8_t size);
void write_rows(activity_raw_t row[], uint8_t size);

char* get_first_record(uint8_t mode);
char* get_next_record();

uint8_t get_record_desc(char* filename, record_desc_t* record);

#endif


