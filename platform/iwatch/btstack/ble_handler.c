
#include "ble_handler.h"

#include <stdio.h>
#include "window.h"
#include "btstack/include/btstack/utils.h"
#include "rtc.h"
#include "stlv_handler.h"

static const ble_handle_t s_ble_handle_table[] = {
    /*     characteristic, name                          type                       size*/
    DEF_BLE_HANDLE("fff1", BLE_HANDLE_TEST_READ,         BLE_HANDLE_TYPE_INT8_ARR,  1),
    DEF_BLE_HANDLE("fff2", BLE_HANDLE_TEST_WRITE,        BLE_HANDLE_TYPE_INT8_ARR,  1),
    DEF_BLE_HANDLE("fff3", BLE_HANDLE_DATETIME,          BLE_HANDLE_TYPE_INT8_ARR,  6),
    DEF_BLE_HANDLE("fff4", BLE_HANDLE_ALARM_0,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    DEF_BLE_HANDLE("fff5", BLE_HANDLE_ALARM_1,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    DEF_BLE_HANDLE("fff6", BLE_HANDLE_ALARM_2,           BLE_HANDLE_TYPE_INT8_ARR,  2),
    DEF_BLE_HANDLE("fff7", BLE_HANDLE_SPORTS_GRID,       BLE_HANDLE_TYPE_INT8_ARR,  4),
    DEF_BLE_HANDLE("fff8", BLE_HANDLE_SPORTS_DATA,       BLE_HANDLE_TYPE_INT32_ARR, 5),
    DEF_BLE_HANDLE("fff9", BLE_HANDLE_SPORTS_DESC,       BLE_HANDLE_TYPE_INT32_ARR, 2),
    DEF_BLE_HANDLE("ff10", BLE_HANDLE_DEVICE_ID,         BLE_HANDLE_TYPE_INT32_ARR, 1),
    DEF_BLE_HANDLE("ff11", BLE_HANDLE_FILE_DESC,         BLE_HANDLE_TYPE_STRING,    32),
    DEF_BLE_HANDLE("ff12", BLE_HANDLE_FILE_DATA,         BLE_HANDLE_TYPE_INT8_ARR,  80),
    DEF_BLE_HANDLE("ff13", BLE_HANDLE_GPS_INFO,          BLE_HANDLE_TYPE_INT16_ARR, 4),
    DEF_BLE_HANDLE("ff14", BLE_HANDLE_CONF_GESTURE,      BLE_HANDLE_TYPE_INT8_ARR,  5),
    DEF_BLE_HANDLE("ff15", BLE_HANDLE_CONF_WORLDCLOCK_0, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff16", BLE_HANDLE_CONF_WORLDCLOCK_1, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff17", BLE_HANDLE_CONF_WORLDCLOCK_2, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff18", BLE_HANDLE_CONF_WORLDCLOCK_3, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff19", BLE_HANDLE_CONF_WORLDCLOCK_4, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff20", BLE_HANDLE_CONF_WORLDCLOCK_5, BLE_HANDLE_TYPE_STRING,    10),
    DEF_BLE_HANDLE("ff21", BLE_HANDLE_CONF_WATCHFACE,    BLE_HANDLE_TYPE_INT8_ARR,  1),
    DEF_BLE_HANDLE("ff22", BLE_HANDLE_CONF_GOALS,        BLE_HANDLE_TYPE_INT16_ARR, 3),
    DEF_BLE_HANDLE("ff23", BLE_HANDLE_CONF_USER_PROFILE, BLE_HANDLE_TYPE_INT8_ARR,  3),
};

static uint8_t s_flag = 0;
static uint8_t s_test = 0;
static uint32_t s_sports_data_buffer[5] = {0};
static uint32_t s_sports_desc_buffer[2] = {0};

static void att_read_world_clock(uint8_t id, char* buf, uint8_t buf_size)
{
    ui_config* conf = window_readconfig();
    if (id >= sizeof(conf->worldclock_offset))
        return;

    if (buf_size <= sizeof(conf->worldclock_name[0]) + 3)
        return;

    sprintf("%s[%d]", conf->worldclock_name[id], conf->worldclock_offset[id]);
}

static void att_write_world_clock(uint8_t id, char* buf)
{
    ui_config* conf = window_readconfig();
    if (id >= sizeof(conf->worldclock_offset))
        return;

    uint8_t i = strlen(buf);
    if (i == 0)
        return;

    uint8_t offset = 0;
    uint8_t shift  = 0;
    for (; i >= 0; --i)
    {
        if (buf[i] == ']')
            continue;

        offset = offset * 10 * shift + buf[i] - '0';

        if (buf[i] == '[')
        {
            buf[i] = '\0';
            break;
        }
    }

    conf->worldclock_offset[id] = offset;
    strcpy(conf->worldclock_name[id], buf);

    window_writeconfig();
    window_loadconfig();

}

uint16_t att_handler(uint16_t handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size, uint8_t mode)
{
    printf("att_handler(handle=%x, buf=%x(%d))\n", handle, buffer, buffer_size);
    const ble_handle_t* hble = get_ble_handle(handle);
    if (hble == NULL)
    {
        printf("No correspondent handler\n");
        return 0;
    }

    uint16_t actual_size = hble->size * get_type_unit_size(hble->type);
    if (buffer == 0 || buffer_size < actual_size)
    {
        printf("Invalid Buffer(addr=%x, size=%d/%d)\n", buffer, buffer_size, actual_size);
        return actual_size;
    }

    switch (handle)
    {
        case BLE_HANDLE_TEST_READ:
        case BLE_HANDLE_TEST_WRITE:
            if (mode == ATT_HANDLE_MODE_READ)
                buffer[0] = s_test;
            else
                s_test = buffer[0];
            break;

        case BLE_HANDLE_SPORTS_GRID:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ui_config* conf = window_readconfig();
                uint8_t i = 0;
                for (; i < conf->sports_grid + 2 &&
                       i < buffer_size &&
                       i + 1 < sizeof(conf->sports_grid_data); ++i)
                {
                    buffer[i] = conf->sports_grid_data[i + 1];
                }
                for (; i < buffer_size; ++i)
                {
                    buffer[i] = 0xff;
                }
            }
            else
            {
                ui_config* conf = window_readconfig();
                uint8_t valid_cnt = 0;
                for (uint8_t i = 0; i < buffer_size && i + 1 < sizeof(conf->sports_grid_data); ++i)
                {
                    if (buffer[i] == 0xff)
                    {
                        valid_cnt = i;
                        break;
                    }
                    else
                    {
                        conf->sports_grid_data[i + 1] = buffer[i];
                    }
                }
                conf->sports_grid = valid_cnt - 2;
                window_writeconfig();
            }
            break;

        case BLE_HANDLE_SPORTS_DATA:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memcpy(buffer, s_sports_data_buffer, sizeof(s_sports_data_buffer));
            }
            else
            {
                for (uint8_t i = 0; i < buffer_size / sizeof(uint32_t); ++i)
                    s_sports_data_buffer[i] = READ_NET_32(&buffer, i * sizeof(uint32_t));
            }
            break;

        case BLE_HANDLE_SPORTS_DESC:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memcpy(buffer, s_sports_desc_buffer, sizeof(s_sports_desc_buffer));
            }
            else
            {
                for (uint8_t i = 0; i < buffer_size / sizeof(uint32_t); ++i)
                    s_sports_desc_buffer[i] = READ_NET_32(&buffer, i * sizeof(uint32_t));
            }
            break;

        case BLE_HANDLE_DATETIME:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                uint16_t year = 0;
                uint8_t  weekday = 0;
                rtc_readdate(&year, &buffer[1], &buffer[2], &weekday);
                rtc_readtime(&buffer[3], &buffer[4], &buffer[5]);
                buffer[0] = year - 2000;
            }
            else
            {
                handle_clock(buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5]);
            }
            break;

        case BLE_HANDLE_ALARM_0:
        case BLE_HANDLE_ALARM_1:
        case BLE_HANDLE_ALARM_2:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size);
            }
            else
            {
                rtc_setalarm(0, 0, buffer[0] | 0x80, buffer[1] | 0x80);
            }
            break;

        case BLE_HANDLE_DEVICE_ID:
        case BLE_HANDLE_FILE_DESC:
        case BLE_HANDLE_FILE_DATA:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size);
            }
            break;

        case BLE_HANDLE_GPS_INFO:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size);
            }
            else
            {
                uint32_t spd  = READ_NET_32(buffer, 0);
                uint32_t alt  = READ_NET_32(buffer, 4);
                uint32_t dist = READ_NET_32(buffer, 8);
                handle_gps_info(spd, alt, dist);
            }
            break;

        case BLE_HANDLE_CONF_GESTURE:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ui_config* conf = window_readconfig();
                buffer[0] = conf->gesture_flag;
                for (uint8_t i = 0; i < sizeof(conf->gesture_map) && i + 1 < buffer_size; ++i)
                {
                    buffer[i + 1] = conf->gesture_map[i];
                }
            }
            else
            {
                handle_gesture_control(buffer[0], &buffer[1]);
            }
            break;

        case BLE_HANDLE_CONF_WORLDCLOCK_0:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(0, (char*)buffer, buffer_size);
            else
                att_write_world_clock(0, (char*)buffer);
            break;
        case BLE_HANDLE_CONF_WORLDCLOCK_1:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(1, (char*)buffer, buffer_size);
            else
                att_write_world_clock(1, (char*)buffer);
            break;
        case BLE_HANDLE_CONF_WORLDCLOCK_2:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(2, (char*)buffer, buffer_size);
            else
                att_write_world_clock(2, (char*)buffer);
            break;
        case BLE_HANDLE_CONF_WORLDCLOCK_3:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(3, (char*)buffer, buffer_size);
            else
                att_write_world_clock(3, (char*)buffer);
            break;
        case BLE_HANDLE_CONF_WORLDCLOCK_4:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(4, (char*)buffer, buffer_size);
            else
                att_write_world_clock(4, (char*)buffer);
            break;
        case BLE_HANDLE_CONF_WORLDCLOCK_5:
            if (mode == ATT_HANDLE_MODE_READ)
                att_read_world_clock(5, (char*)buffer, buffer_size);
            else
                att_write_world_clock(5, (char*)buffer);
            break;

        case BLE_HANDLE_CONF_WATCHFACE:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ui_config* conf = window_readconfig();
                buffer[0] = conf->default_clock;
                buffer[1] = conf->default_clock == 0 ? conf->analog_clock : conf->digit_clock;
            }
            else
            {
                ui_config* conf = window_readconfig();
                conf->default_clock = buffer[0];
                if (buffer[0] == 0)
                    conf->analog_clock = buffer[1];
                else
                    conf->digit_clock = buffer[1];
                window_writeconfig();
                window_loadconfig();
            }
            break;

        case BLE_HANDLE_CONF_GOALS:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ui_config* conf = window_readconfig();
                uint8_t i = 0;
                memcpy(&buffer[i], &conf->goal_steps, sizeof(conf->goal_steps));
                i += sizeof(conf->goal_steps);

                memcpy(&buffer[i], &conf->goal_distance, sizeof(conf->goal_distance));
                i += sizeof(conf->goal_distance);

                memcpy(&buffer[i], &conf->goal_calories, sizeof(conf->goal_calories));
                i += sizeof(conf->goal_calories);
            }
            else
            {
                ui_config* conf = window_readconfig();

                conf->goal_steps    = READ_NET_16(buffer, 0);
                conf->goal_distance = READ_NET_16(buffer, 2);
                conf->goal_calories = READ_NET_16(buffer, 4);

                window_writeconfig();
                window_loadconfig();
            }
            break;

        case BLE_HANDLE_CONF_USER_PROFILE:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ui_config* conf = window_readconfig();
                buffer[0] = conf->height;
                buffer[1] = conf->weight;
                buffer[2] = conf->circumference;
            }
            else
            {
                ui_config* conf = window_readconfig();

                conf->height        = buffer[0];
                conf->weight        = buffer[1];
                conf->circumference = buffer[2];

                window_writeconfig();
                window_loadconfig();
            }
            break;

        case BLE_HANDLE_RESERVED_0:
        case BLE_HANDLE_RESERVED_1:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size);
            }
            break;
    }
    return actual_size;
}

uint8_t get_type_unit_size(uint8_t type)
{
    static uint8_t s_uint_size_table[] = {1, 2, 4, 1};
    return s_uint_size_table[type];
}

const ble_handle_t* get_ble_handle(uint16_t handle)
{
    uint16_t offset = (handle - s_ble_handle_table[0].handle) / 2;
    if (offset < sizeof(s_ble_handle_table) / sizeof(s_ble_handle_table[0]))
        return &s_ble_handle_table[offset];
    else
        return NULL;
}

