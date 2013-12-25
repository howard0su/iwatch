
#ifndef _BLE_HANDLER_H_
#define _BLE_HANDLER_H_

#include "stdint.h"

#define BLE_HANDLE_TEST_READ         0x000b
#define BLE_HANDLE_TEST_WRITE        0x000d
#define BLE_HANDLE_DATETIME          0x000f
#define BLE_HANDLE_ALARM_0           0x0011
#define BLE_HANDLE_ALARM_1           0x0013
#define BLE_HANDLE_ALARM_2           0x0015
#define BLE_HANDLE_SPORTS_GRID       0x0017
#define BLE_HANDLE_SPORTS_DATA       0x0019
#define BLE_HANDLE_SPORTS_DESC       0x001b
#define BLE_HANDLE_DEVICE_ID         0x001d
#define BLE_HANDLE_FILE_DESC         0x001f
#define BLE_HANDLE_FILE_DATA         0x0021
#define BLE_HANDLE_GPS_INFO          0x0023
#define BLE_HANDLE_CONF_GESTURE      0x0025
#define BLE_HANDLE_CONF_WORLDCLOCK_0 0x0027
#define BLE_HANDLE_CONF_WORLDCLOCK_1 0x0029
#define BLE_HANDLE_CONF_WORLDCLOCK_2 0x002b
#define BLE_HANDLE_CONF_WORLDCLOCK_3 0x002d
#define BLE_HANDLE_CONF_WORLDCLOCK_4 0x002f
#define BLE_HANDLE_CONF_WORLDCLOCK_5 0x0031
#define BLE_HANDLE_CONF_WATCHFACE    0x0033
#define BLE_HANDLE_CONF_GOALS        0x0035
#define BLE_HANDLE_CONF_USER_PROFILE 0x0037
#define BLE_HANDLE_RESERVED_0        0x0039
#define BLE_HANDLE_RESERVED_1        0x003b

#define BLE_HANDLE_TYPE_INT8_ARR   0
#define BLE_HANDLE_TYPE_INT16_ARR  1
#define BLE_HANDLE_TYPE_INT32_ARR  2
#define BLE_HANDLE_TYPE_STRING     3

#define DEF_BLE_HANDLE(uuid, name, type, size) {name, type, size, 0}

typedef struct _ble_handle_t
{
//    const char* characteristic;
//    const char* name;
    uint8_t handle;
    uint8_t  type;
    uint8_t  size;
    uint8_t  offset;
}ble_handle_t;

ble_handle_t* get_ble_handle(uint16_t handle);
uint8_t get_type_unit_size(uint8_t type);
void create_ble_handle_db();
uint8_t* get_handle_buf(uint16_t handle);

char*     read_string(uint16_t handle);
uint8_t   read_uint8(uint16_t handle, uint8_t default_value);
uint8_t*  read_uint8_array(uint16_t handle);
uint16_t  read_uint16(uint16_t handle, uint16_t default_value);
uint16_t* read_uint16_array(uint16_t handle);
uint32_t  read_uint32(uint16_t handle, uint32_t default_value);
uint32_t* read_uint32_array(uint16_t handle);

int write_string(uint16_t handle, char* str);
int write_uint8(uint16_t handle, uint8_t value);
int write_uint8_array(uint16_t handle, uint8_t* array, uint8_t size);
int write_uint16(uint16_t handle, uint16_t value);
int write_uint16_array(uint16_t handle, uint16_t* array, uint8_t szie);
int write_uint32(uint16_t handle, uint32_t value);
int write_uint32_array(uint16_t handle, uint32_t* array, uint8_t size);

#endif

