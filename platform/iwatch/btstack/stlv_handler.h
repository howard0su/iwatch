

#ifndef _STLV_HANDLER_H_
#define _STLV_HANDLER_H_

#include <stdint.h>
#include "stlv_client.h"
#include "contiki.h"
#include "window.h"

//-----------------------message handlers----------------------
void handle_echo(uint8_t* data, int size);
void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void handle_message(uint8_t msg_type, char* ident, char* message);

//-----------------------data----------------------

void handle_get_file(char* name);
void handle_list_file();
void handle_remove_file(char* name);

//-----------------------file handlers----------------------
int handle_file_begin(char* name);
int handle_file_data(int fd, uint8_t* data, uint8_t size);
void handle_file_end(int fd);

int transfer_file(char* name);
void handle_get_sports_data(uint16_t *data, uint8_t numofdata);
void handle_get_sports_grid();
void handle_set_sports_grid();

//-----------------------alarm handlers---------------------
void handle_alarm(alarm_conf_t* para);

void handle_get_device_id();

void handle_gps_info(uint16_t spd, uint16_t alt, uint32_t distance, uint16_t calories);

#define GESTURE_FLAG_ENABLE 0x01
#define GESTURE_FLAG_LEFT   0x02
void handle_gesture_control(uint8_t flag, uint8_t action_map[]);

void handle_set_watch_config(ui_config* conf);

#endif

