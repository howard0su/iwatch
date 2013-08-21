
#ifndef _STLV_CLIENT_H_
#define _STLV_CLIENT_H_

#include <stdint.h>

//-----------------------message sender------------------------
void send_echo(uint8_t* data, uint8_t size);

//return 0 if success
int begin_send_file(char* name);

void send_file_data(char* name, uint8_t* data, uint8_t size, void (*data_sent)(char* name));

void end_send_file(char* name);

#endif
