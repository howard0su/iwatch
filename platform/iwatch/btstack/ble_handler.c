
#include "ble_handler.h"

#include <stdio.h>
#include <string.h>

#include "contiki.h"
#include "window.h"
#include "btstack/include/btstack/utils.h"
#include "rtc.h"
#include "stlv_handler.h"
#include "btstack-config.h"
#include "debug.h"
#include "watch/sportsdata.h"
#include "cfs/cfs.h"

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
    DEF_BLE_HANDLE("ff11", BLE_HANDLE_FILE_DESC,         BLE_HANDLE_TYPE_STRING,    20),
    DEF_BLE_HANDLE("ff12", BLE_HANDLE_FILE_DATA,         BLE_HANDLE_TYPE_INT8_ARR,  20),
    DEF_BLE_HANDLE("ff13", BLE_HANDLE_GPS_INFO,          BLE_HANDLE_TYPE_INT16_ARR, 4),
    DEF_BLE_HANDLE("ff14", BLE_HANDLE_CONF_GESTURE,      BLE_HANDLE_TYPE_INT8_ARR,  5),
    DEF_BLE_HANDLE("ff15", BLE_HANDLE_CONF_WORLDCLOCK_0, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff16", BLE_HANDLE_CONF_WORLDCLOCK_1, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff17", BLE_HANDLE_CONF_WORLDCLOCK_2, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff18", BLE_HANDLE_CONF_WORLDCLOCK_3, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff19", BLE_HANDLE_CONF_WORLDCLOCK_4, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff20", BLE_HANDLE_CONF_WORLDCLOCK_5, BLE_HANDLE_TYPE_STRING,    10 + 4),
    DEF_BLE_HANDLE("ff21", BLE_HANDLE_CONF_WATCHFACE,    BLE_HANDLE_TYPE_INT8_ARR,  1),
    DEF_BLE_HANDLE("ff22", BLE_HANDLE_CONF_GOALS,        BLE_HANDLE_TYPE_INT16_ARR, 3),
    DEF_BLE_HANDLE("ff23", BLE_HANDLE_CONF_USER_PROFILE, BLE_HANDLE_TYPE_INT8_ARR,  3),
};

static uint8_t s_test = 0;

static uint32_t s_sports_data_buffer[5] = {0};
static uint32_t s_sports_desc_buffer[2] = {0};

//reset
#define FD_REST           'X'

//for send firmware
#define FD_INVESTIGATE    'I'
#define FD_FILE_FOUND     'F'
#define FD_NO_FILE        'N'
#define FD_READ_FILE      'R'
#define FD_DATA_TRAN      'D'
#define FD_END_OF_DATA    'E'

//for read data file
#define FD_WRITE_FILE     'W'
#define FD_WRITE_HANDLED  'H'
#define FD_SEND_DATA      'S'
#define FD_BLOCK_PREPARED 'P'
#define FD_BLOCK_OVER     'O'
#define FD_SEND_COMPLETE  'C'

//file transferring status
#define FS_IDLE          0x00
#define FS_INVESTIGATING 0x01
#define FS_READING       0x02
#define FS_NO_DATA       0x03
#define FS_FILE_FOUND    0x04

#define FS_WRITE         0x10
#define FS_WF_PREPARED   0x20
#define FS_SENDING       0x30
#define FS_SEND_OK       0x40

static uint8_t s_file_mode = FS_IDLE;
static char s_file_desc[20] = "";
static int s_read_fd = -1;
static int s_write_fd = -1;
static uint16_t s_block_id = 0xffff;
static uint8_t s_file_data[20] = "";

static void init_ble_file_handler()
{
    s_file_mode = FS_IDLE;
    memset(s_file_desc, 0, sizeof(s_file_desc));
    memset(s_file_data, 0, sizeof(s_file_desc));

    s_block_id = 0xffff;
    if (s_read_fd != -1)
    {
        cfs_close(s_read_fd);
        s_read_fd = -1;
    }
}

#define FD_GET_COMMAND(buf)   (buf[0])
#define FD_GET_BLOCKID(buf)   *((uint16_t*)&buf[2])
#define FD_GET_BLOCKSIZE(buf) (buf[1])
#define FD_GET_FILENAME(buf)  ((char*)&buf[4])

#define FD_SET_COMMAND(buf, cmd)          FD_GET_COMMAND(buf) = cmd
#define FD_SET_BLOCKSIZE(buf, blocksize)  FD_GET_BLOCKSIZE(buf) = blocksize
#define FD_SET_FILENAME(buf, filename)    strcpy(&buf[4], filename)

void FD_SET_BLOCKID(uint8_t* buf, uint16_t blockid)
{
    memcpy(&buf[2], &blockid, 2);
}

static void ble_write_file_desc(uint8_t* buffer, uint16_t buffer_size)
{
    log_info("ble_write_file_desc() mode=%x\n", s_file_mode);
    if (FD_GET_COMMAND(buffer) == FD_REST)
    {
        init_ble_file_handler();
        return;
    }

    switch(s_file_mode)
    {
        case FS_INVESTIGATING:
        break;

        case FS_IDLE:
        if (FD_GET_COMMAND(buffer) == FD_INVESTIGATE)
        {
            s_file_mode = FS_INVESTIGATING;
            return;
        }

        if (FD_GET_COMMAND(buffer) == FD_WRITE_FILE)
        {
            s_file_mode = FS_WRITE;
            return;
        }
        break;

        case FS_WRITE:
        if (FD_GET_COMMAND(buffer) == FD_SEND_DATA)
        {
            memcpy(s_file_desc, buffer, buffer_size);
            s_file_mode = FS_WF_PREPARED;
            s_block_id = 0;
            return;
        }
        break;

        case FS_WF_PREPARED:
        if (FD_GET_COMMAND(buffer) == FD_SEND_DATA)
        {
            char* newfilename = FD_GET_FILENAME(buffer);
            char* oldfilename = FD_GET_FILENAME(s_file_desc);

            uint16_t newblockid = FD_GET_BLOCKID(buffer);
            uint16_t oldblockid = FD_GET_BLOCKID(s_file_desc);

            if (strcmp(newfilename, oldfilename) != 0 || newblockid != oldblockid)
                init_ble_file_handler();
            else
                FD_SET_COMMAND(s_file_desc, FD_SEND_DATA);
            return;
        }
        break;

        case FS_SENDING:
        if (FD_GET_COMMAND(buffer) == FD_SEND_DATA)
        {
            memcpy(s_file_desc, buffer, sizeof(s_file_desc));
            s_file_mode = FS_WF_PREPARED;
            return;
        }

        if (FD_GET_COMMAND(buffer) == FD_SEND_COMPLETE)
        {
            handle_file_end(s_write_fd);
            s_write_fd = -1;

            init_ble_file_handler();
            s_file_mode = FS_IDLE;
            return;
        }
        break;

        case FS_FILE_FOUND:
        if (FD_GET_COMMAND(buffer) == FD_READ_FILE)
        {
            memcpy(s_file_desc, buffer, sizeof(s_file_desc));
            s_file_mode = FS_READING;
            return;
        }
        break;

        case FS_READING:
        if (FD_GET_COMMAND(buffer) == FD_READ_FILE)
        {
            memcpy(s_file_desc, buffer, sizeof(s_file_desc));
            return;
        }
        break;
    }

}

static void ble_read_file_desc(uint8_t * buffer, uint16_t buffer_size)
{
    log_info("ble_read_file_desc() mode=%x\n", s_file_mode);
    switch (s_file_mode)
    {
        case FS_IDLE:
        case FS_NO_DATA:
        case FS_FILE_FOUND:
            //do nothing
            break;

        // for data upload
        case FS_INVESTIGATING:
        {
            char* name = get_data_file();
            if (name == NULL)
            {
                FD_SET_COMMAND(s_file_desc, FD_NO_FILE);
                s_file_mode = FS_IDLE;
            }
            else
            {
                FD_SET_COMMAND(s_file_desc, FD_FILE_FOUND);
                FD_SET_BLOCKID((uint8_t*)s_file_desc, 0);
                FD_SET_BLOCKSIZE(s_file_desc, 0);
                FD_SET_FILENAME(s_file_desc, name);
                s_file_mode = FS_FILE_FOUND;
            }
        }
        break;

        case FS_READING:
        {
            uint16_t blockid = FD_GET_BLOCKID(s_file_desc);
            if (s_read_fd == -1)
            {
                char* filename = FD_GET_FILENAME(s_file_desc);
                s_read_fd = cfs_open(filename, CFS_READ);
                if (s_read_fd == -1)
                {
                    log_error("cfs_open(%s) failed when FS_READING\n", filename);
                    FD_SET_COMMAND(s_file_desc, FD_END_OF_DATA);
                    s_file_mode = FS_IDLE;
                    break;
                }
            }

            if (s_block_id != blockid)
            {
                uint16_t block_offset = blockid * sizeof(s_file_data);
                cfs_offset_t seek_ret = cfs_seek(s_read_fd, block_offset, CFS_SEEK_SET);
                if (seek_ret == -1)
                {
                    log_error("cfs_seek(%d, %d(=%d*%d)) failed when FS_READING\n", s_read_fd, block_offset, s_block_id, sizeof(s_file_data));
                    FD_SET_COMMAND(s_file_desc, FD_END_OF_DATA);
                    s_file_mode = FS_IDLE;
                    break;
                }

                int read_byte = cfs_read(s_read_fd, s_file_data, sizeof(s_file_data));
                if (read_byte == 0)
                {
                    log_error("cfs_read(%d) failed when FS_READING\n", s_read_fd);
                    FD_SET_COMMAND(s_file_desc, FD_END_OF_DATA);
                    s_file_mode = FS_IDLE;
                    break;
                }

                s_block_id = blockid;

                FD_SET_BLOCKSIZE(s_file_desc, read_byte);
            }

            FD_SET_COMMAND(s_file_desc, FD_DATA_TRAN);

        }
        break;

        //for fw upgrade
        case FS_WRITE:
            FD_SET_COMMAND(s_file_desc, FD_WRITE_HANDLED);
            break;

        case FS_WF_PREPARED:
            FD_SET_COMMAND(s_file_desc, FD_BLOCK_PREPARED);
            break;

        case FS_SENDING:
            FD_SET_COMMAND(s_file_desc, FD_BLOCK_OVER);
            break;

    }

    memcpy(buffer, s_file_desc, buffer_size);
}

static void ble_read_file_data(uint8_t* buffer, uint8_t buffer_size)
{
    if (s_file_mode == FS_READING)
    {
        memcpy(buffer, s_file_data, buffer_size);
    }
    else
    {
        memset(buffer, 0, buffer_size);
    }
}

static void ble_write_file_data(uint8_t* buffer, uint8_t buffer_size)
{
    if (s_file_mode != FS_WF_PREPARED)
        return;

    uint16_t blockid = FD_GET_BLOCKID(s_file_desc);
    if (s_write_fd == -1)
    {
        char* filename = FD_GET_FILENAME(s_file_desc);
        s_write_fd = handle_file_begin(filename);
        if (s_write_fd == -1)
            return;
    }

    if (blockid > s_block_id + 1)
    {
        init_ble_file_handler();
        return;
    }
    else if (blockid < s_block_id)
    {
        return;
    }
    else
    {
        uint8_t size = FD_GET_BLOCKSIZE(s_file_desc);
        if (size > buffer_size)
        {
            init_ble_file_handler();
            return;
        }
        handle_file_data(s_write_fd, buffer, size);

        s_block_id = blockid + 1;
        s_file_mode = FS_SENDING;
    }

}
static void att_read_world_clock(uint8_t id, char* buf, uint8_t buf_size)
{
    ui_config* conf = window_readconfig();
    if (id >= sizeof(conf->worldclock_offset))
        return;

    if (buf_size <= sizeof(conf->worldclock_name[0]) + 3)
        return;

    sprintf(buf, "%s[%d]", conf->worldclock_name[id], conf->worldclock_offset[id]);
    log_info("Read World clock:\n");
    log_info(buf);
    log_info("\n");
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
    for (; i > 0; --i)
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
    //log_info("att_handler(handle=%x, buflen=%d)\n", handle, buffer_size);
    const ble_handle_t* hble = get_ble_handle(handle);
    if (hble == NULL)
    {
        log_info("No correspondent handler\n");
        return 0;
    }

    uint16_t actual_size = hble->size * get_type_unit_size(hble->type);
    if (buffer == 0 || buffer_size < actual_size)
    {
        log_info("Invalid Buffer(size=%d/%d)\n", buffer_size, actual_size);
        return actual_size;
    }

    switch (handle)
    {
        case BLE_HANDLE_TEST_READ:
        case BLE_HANDLE_TEST_WRITE:
            log_info("BLE Test Characteristic\n");
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
                uint8_t i = 0;
                for (; i < buffer_size && i + 1 < sizeof(conf->sports_grid_data); ++i)
                {
                    if (buffer[i] == 0xff)
                    {
                        break;
                    }
                    else
                    {
                        conf->sports_grid_data[i + 1] = buffer[i];
                    }
                }
                if (i < 2) i = 2;
                conf->sports_grid = i - 2;
                window_writeconfig();
                window_loadconfig();
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
            log_info("Set Alarm()\n");
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size); //no way read them back so far
            }
            else
            {
                rtc_setalarm(0, 0, buffer[0] | 0x80, buffer[1] | 0x80);
            }
            break;

        case BLE_HANDLE_DEVICE_ID:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                memset(buffer, 0, buffer_size);
            }
            break;

        case BLE_HANDLE_FILE_DESC:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ble_read_file_desc(buffer, buffer_size);
            }
            else
            {
                ble_write_file_desc(buffer, buffer_size);
            }
            break;

        case BLE_HANDLE_FILE_DATA:
            if (mode == ATT_HANDLE_MODE_READ)
            {
                ble_read_file_data(buffer, buffer_size);
            }
            else
            {
                ble_write_file_data(buffer, buffer_size);
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
    for (uint8_t i = 0; i < sizeof(s_ble_handle_table)/sizeof(s_ble_handle_table[0]); ++i)
    {
        if (s_ble_handle_table[i].handle == handle)
            return &s_ble_handle_table[i];
    }
    return NULL;
    //uint16_t offset = (handle - s_ble_handle_table[0].handle) / 2;
    //if (offset < sizeof(s_ble_handle_table) / sizeof(s_ble_handle_table[0]))
    //    return &s_ble_handle_table[offset];
    //else
    //    return NULL;
}

void ble_start_sync(uint8_t mode)
{
    s_sports_desc_buffer[0] = mode;
}

void ble_send_sports_data(uint32_t time, uint32_t data[], uint8_t size)
{
    s_sports_data_buffer[0] = time;
    for (uint8_t i = 1; i < size && i < count_elem(s_sports_data_buffer); ++i)
        s_sports_data_buffer[i] = data[i];
}

void ble_send_normal_data(uint32_t time, uint32_t steps, uint32_t cals, uint32_t dist)
{
    s_sports_data_buffer[0] = time;
    s_sports_data_buffer[1] = steps;
    s_sports_data_buffer[2] = steps;
    s_sports_data_buffer[3] = steps;
}

void ble_stop_sync()
{
    s_sports_desc_buffer[0] = 0;
}
