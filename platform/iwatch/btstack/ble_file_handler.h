#ifndef _BLE_HANDLER_FILE_H_
#define _BLE_HANDLER_FILE_H_

#include "stdint.h"

void ble_write_file_desc(uint8_t* buffer, uint16_t buffer_size);
void ble_read_file_desc(uint8_t * buffer, uint16_t buffer_size);
void ble_read_file_data(uint8_t* buffer, uint8_t buffer_size);
void ble_write_file_data(uint8_t* buffer, uint8_t buffer_size);

#endif
