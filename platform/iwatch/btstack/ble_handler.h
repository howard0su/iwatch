
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

#define DEF_BLE_HANDLE(uuid, name, type, size) {uuid, #name, name, type, size}

typedef struct _ble_handle_t
{
    const char* characteristic;
    const char* name;
    const uint8_t handle;
    const uint8_t type;
    const uint8_t size;
}ble_handle_t;

const ble_handle_t* get_ble_handle(uint16_t handle);
uint8_t get_type_unit_size(uint8_t type);

#define ATT_HANDLE_MODE_READ  0x00
#define ATT_HANDLE_MODE_WRITE 0x01
uint16_t gatt_handler(uint16_t handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size, uint8_t mode);

#define SPORTS_SYNC_MODE_IDEL   0x00
#define SPORTS_SYNC_MODE_SPORTS 0x01
#define SPORTS_SYNC_MODE_SYNC   0x02
void ble_start_sync(uint8_t mode);
void ble_send_running_data(uint32_t time, uint32_t steps, uint32_t cals, uint32_t dist, uint32_t heart);
void ble_send_biking_data(uint32_t time, uint32_t steps, uint32_t cals, uint32_t dist, uint32_t heart);
void ble_send_normal_data(uint32_t time, uint32_t steps, uint32_t cals, uint32_t dist);
void ble_stop_sync();
#endif

