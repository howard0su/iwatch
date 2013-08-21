

#ifndef _STLV_SERVER_H_
#define _STLV_SERVER_H_

#include <stdint.h>

//-----------------------message handlers----------------------
void handle_echo(uint8_t* data, int size);
void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second);
void handle_message(uint8_t msg_type, char* ident, char* message);

void handle_file_begin(char* name);
void handle_file_data(char* name, uint8_t* data, uint8_t size);
void handle_file_end(char* name);

#endif

