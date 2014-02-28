
#ifndef _SPORTSDATE_H_
#define _SPORTSDATE_H_

#include <stdint.h>

//for running/biking store interval is 3 seconds
// the 2400 can hold 2 hours and about 46k in bytes
#define MAX_ROW_COUNT 2400

#define DATA_MODE_NORMAL    0x00
#define DATA_MODE_RUNNING   0x01
#define DATA_MODE_BIKING    0x02
#define DATA_MODE_WALKING   0x03
#define DATA_MODE_PAUSED    0x10
#define DATA_MODE_TOOMSTONE 0xFF

#define DATA_COL_INVALID 0x00
#define DATA_COL_STEP    0x01
#define DATA_COL_DIST    0x02
#define DATA_COL_CALS    0x03
#define DATA_COL_CADN    0x04
#define DATA_COL_HR      0x05

int create_data_file(uint8_t year, uint8_t month, uint8_t day);
void write_data_line(uint8_t mode, uint8_t hh, uint8_t mm, uint8_t meta[], uint32_t data[], uint8_t size);
void close_data_file();

char* get_data_file();
void remove_data_file(char* filename);
uint32_t build_data_schema(uint8_t coltype[], uint8_t colcount);

uint8_t set_mode(uint8_t mode);
uint8_t get_mode();

#endif


