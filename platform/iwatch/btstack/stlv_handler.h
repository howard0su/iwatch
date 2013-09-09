

#ifndef _STLV_HANDLER_H_
#define _STLV_HANDLER_H_

#include <stdint.h>

//-----------------------message handlers----------------------
void handle_echo(uint8_t* data, int size);
void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void handle_message(uint8_t msg_type, char* ident, char* message);

//-----------------------data----------------------
void handle_sports_heartbeat(uint8_t seconds_to_next);

//TODO: sujun provides this file name/prefix
#define FILE_NAME_FILE_LIST
#define FILE_NAME_UI_CONF
#define FILE_NAME_SPORT_DATA
#define FILE_NAME_WATCH_STATUS
#define FILE_PREFIX_GESTURE
#define FILE_PREFIX_WATCHFACE
#define FILE_PREFIX_GADGIT
void handle_get_file(char* name);
void handle_list_file();
void handle_remove_file(char* name);

//-----------------------file handlers----------------------
int handle_file_begin(char* name);
int handle_file_data(int fd, uint8_t* data, uint8_t size);
void handle_file_end(int fd);

int transfer_file(char* name);
void handle_get_sports_data();
void handle_get_sports_grid();

#endif

