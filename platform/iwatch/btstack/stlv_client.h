
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

#endif

