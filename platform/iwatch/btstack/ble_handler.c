
#include "ble_handler.h"

#include <stdio.h>

static ble_handle_t s_ble_handle_table[] = {
    /*     characteristic, name                          type                       size*/
    DEF_BLE_HANDLE("fff1", BLE_HANDLE_TEST_READ,         BLE_HANDLE_TYPE_INT8_ARR,  1),
    DEF_BLE_HANDLE("fff2", BLE_HANDLE_TEST_WRITE,        BLE_HANDLE_TYPE_INT8_ARR,  1),
    //DEF_BLE_HANDLE("fff3", BLE_HANDLE_DATETIME,          BLE_HANDLE_TYPE_INT32_ARR, 1),
    //DEF_BLE_HANDLE("fff4", BLE_HANDLE_ALARM_0,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    //DEF_BLE_HANDLE("fff5", BLE_HANDLE_ALARM_1,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    //DEF_BLE_HANDLE("fff6", BLE_HANDLE_ALARM_2,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    //DEF_BLE_HANDLE("fff7", BLE_HANDLE_SPORTS_GRID,       BLE_HANDLE_TYPE_INT8_ARR,  4),
    //DEF_BLE_HANDLE("fff8", BLE_HANDLE_SPORTS_DATA,       BLE_HANDLE_TYPE_INT32_ARR, 5),
    //DEF_BLE_HANDLE("fff9", BLE_HANDLE_SPORTS_DESC,       BLE_HANDLE_TYPE_INT32_ARR, 2),
    //DEF_BLE_HANDLE("ff10", BLE_HANDLE_DEVICE_ID,         BLE_HANDLE_TYPE_INT32_ARR, 1),
    //DEF_BLE_HANDLE("ff11", BLE_HANDLE_FILE_DESC,         BLE_HANDLE_TYPE_STRING,    32),
    //DEF_BLE_HANDLE("ff12", BLE_HANDLE_FILE_DATA,         BLE_HANDLE_TYPE_INT8_ARR,  80),
    //DEF_BLE_HANDLE("ff13", BLE_HANDLE_GPS_INFO,          BLE_HANDLE_TYPE_INT16_ARR, 4),
    //DEF_BLE_HANDLE("ff14", BLE_HANDLE_CONF_GESTURE,      BLE_HANDLE_TYPE_INT8_ARR,  5),
    //DEF_BLE_HANDLE("ff15", BLE_HANDLE_CONF_WORLDCLOCK_0, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff16", BLE_HANDLE_CONF_WORLDCLOCK_1, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff17", BLE_HANDLE_CONF_WORLDCLOCK_2, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff18", BLE_HANDLE_CONF_WORLDCLOCK_3, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff19", BLE_HANDLE_CONF_WORLDCLOCK_4, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff20", BLE_HANDLE_CONF_WORLDCLOCK_5, BLE_HANDLE_TYPE_STRING,    10),
    //DEF_BLE_HANDLE("ff21", BLE_HANDLE_CONF_WATCHFACE,    BLE_HANDLE_TYPE_INT8_ARR,  1),
    //DEF_BLE_HANDLE("ff22", BLE_HANDLE_CONF_GOALS,        BLE_HANDLE_TYPE_INT16_ARR, 3),
    //DEF_BLE_HANDLE("ff23", BLE_HANDLE_CONF_USER_PROFILE, BLE_HANDLE_TYPE_INT8_ARR,  2),
};

static uint8_t s_ble_handle_db[256];
static uint8_t s_flag = 0;
void create_ble_handle_db()
{
    if (s_flag != 0)
    {
        return;
    }
    else
    {
        s_flag = 1;
    }

    for (uint16_t i = 0; i < sizeof(s_ble_handle_db); ++i)
        s_ble_handle_db[i] = 0;

    for (uint16_t i = 1; i < sizeof(s_ble_handle_table) / sizeof(s_ble_handle_table[0]); ++i)
    {
        s_ble_handle_table[i].offset = s_ble_handle_table[i - 1].offset + s_ble_handle_table[i - 1].size;
    }
}

uint8_t get_type_unit_size(uint8_t type)
{
    static uint8_t s_uint_size_table[] = {1, 2, 4, 1};
    return s_uint_size_table[type];
}

ble_handle_t* get_ble_handle(uint16_t handle)
{
    uint16_t offset = (handle - s_ble_handle_table[0].handle) / 2;
    if (offset < sizeof(s_ble_handle_table) / sizeof(s_ble_handle_table[0]))
        return &s_ble_handle_table[offset];
    else
        return NULL;
}

uint8_t* get_handle_buf(uint16_t handle)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return NULL;
    else
        return &s_ble_handle_db[handle_desc->offset];

}

char* read_string(uint16_t handle)
{
    return (char*)read_uint8_array(handle);
}

uint8_t read_uint8(uint16_t handle, uint8_t default_value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return default_value;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return default_value;

    return s_ble_handle_db[handle_desc->offset];
}

uint8_t* read_uint8_array(uint16_t handle)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return NULL;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return NULL;

    return &s_ble_handle_db[handle_desc->offset];
}

uint16_t read_uint16(uint16_t handle, uint16_t default_value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return default_value;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT16_ARR)
        return default_value;

    return *((uint16_t*)(&s_ble_handle_db[handle_desc->offset]));
}

uint16_t* read_uint16_array(uint16_t handle)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return NULL;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return NULL;

    return (uint16_t*)(&s_ble_handle_db[handle_desc->offset]);
}

uint32_t read_uint32(uint16_t handle, uint32_t default_value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return default_value;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT16_ARR)
        return default_value;

    return *((uint32_t*)(&s_ble_handle_db[handle_desc->offset]));
}

uint32_t* read_uint32_array(uint16_t handle)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL)
        return NULL;

    if (handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return NULL;

    return (uint32_t*)(&s_ble_handle_db[handle_desc->offset]);
}


int write_string(uint16_t handle, char* str)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_STRING)
        return -1;

    int i = 0;
    char* pbuf = (char*)&s_ble_handle_db[handle_desc->offset];
    memset(pbuf, 0, handle_desc->size);

    while (str[i] != '\0')
    {
        if (i >= handle_desc->size)
            return i;
        pbuf[i] = str[i];
        ++i;
    }
    return i;
}

int write_uint8(uint16_t handle, uint8_t value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return -1;

    uint8_t* pbuf = &s_ble_handle_db[handle_desc->offset];
    *pbuf = value;
    return 1;
}

int write_uint8_array(uint16_t handle, uint8_t* array, uint8_t size)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT8_ARR)
        return -1;

    uint8_t* pbuf = &s_ble_handle_db[handle_desc->offset];
    uint8_t to_write = size > handle_desc->size ? handle_desc->size : size;
    for (uint8_t i = 0; i < to_write; ++i)
        pbuf[i] = array[i];
    return to_write;
}

int write_uint16(uint16_t handle, uint16_t value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT16_ARR)
        return -1;

    uint16_t* pbuf = (uint16_t*)&s_ble_handle_db[handle_desc->offset];
    *pbuf = value;
    return 1;
}

int write_uint16_array(uint16_t handle, uint16_t* array, uint8_t size)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT16_ARR)
        return -1;

    uint16_t* pbuf = (uint16_t*)&s_ble_handle_db[handle_desc->offset];
    uint8_t to_write = size > handle_desc->size ? handle_desc->size : size;
    for (uint8_t i = 0; i < to_write; ++i)
        pbuf[i] = array[i];
    return to_write;
}

int write_uint32(uint16_t handle, uint32_t value)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT32_ARR)
        return -1;

    uint32_t* pbuf = (uint32_t*)&s_ble_handle_db[handle_desc->offset];
    *pbuf = value;
    return 1;
}

int write_uint32_array(uint16_t handle, uint32_t* array, uint8_t size)
{
    ble_handle_t* handle_desc = get_ble_handle(handle);
    if (handle_desc == NULL || handle_desc->type != BLE_HANDLE_TYPE_INT32_ARR)
        return -1;

    uint32_t* pbuf = (uint32_t*)&s_ble_handle_db[handle_desc->offset];
    uint8_t to_write = size > handle_desc->size ? handle_desc->size : size;
    for (uint8_t i = 0; i < to_write; ++i)
        pbuf[i] = array[i];
    return to_write;
}
