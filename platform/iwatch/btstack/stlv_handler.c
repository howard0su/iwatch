
#include "stlv_handler.h"

#include <stdio.h>
#include <string.h>

#include "stlv.h"
#include "contiki.h"
#include "window.h"
#include "rtc.h"
#include "cfs/cfs.h"
#include "stlv_client.h"
#include "btstack/include/btstack/utils.h"

void handle_echo(uint8_t* data, int data_len)
{
    send_echo(data, data_len);
}

void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_setdate(2000 + year, month, day);
    rtc_settime(hour, minute, second);

    //for test
    //struct cfs_dir dir;
    //int ret = cfs_opendir(&dir, "");
    //if (ret == -1)
    //{
    //    printf("cfs_opendir() failed: %d\n", ret);
    //    return;
    //}

    //while (ret != -1)
    //{
    //    struct cfs_dirent dirent;
    //    ret = cfs_readdir(&dir, &dirent);
    //    if (ret != -1)
    //    {
    //        printf("file:%s, %d\n", dirent.name, dirent.size);
    //    }
    //}

    //cfs_closedir(&dir);
}

#define ICON_FACEBOOK 's'
#define ICON_TWITTER  't'
#define ICON_MSG      'u'

void handle_message(uint8_t msg_type, char* ident, char* message)
{
    uint8_t icon;
    switch (msg_type)
    {
    case ELEMENT_TYPE_MESSAGE_SMS:
        icon = ICON_MSG;
        break;
    case ELEMENT_TYPE_MESSAGE_FB:
        icon = ICON_FACEBOOK;
        break;
    case ELEMENT_TYPE_MESSAGE_TW:
        icon = ICON_TWITTER;
        break;

    case ELEMENT_TYPE_MESSAGE_WEATHER:
    case ELEMENT_TYPE_MESSAGE_BATTERY:
    case ELEMENT_TYPE_MESSAGE_CALL:
    case ELEMENT_TYPE_MESSAGE_REMINDER:
        //TODO: impelment all these
        icon = ICON_MSG;
        break;
    case ELEMENT_TYPE_MESSAGE_RANGE:
        //TODO: this is special: phone out of range is merely an option
        //in message, if the value is "off" turn off the alarm
        //in message, if the value is "on" turn on the alarm
        break;
    default:
        return;
        break;
    }

    window_notify(ident, message, NOTIFY_OK, icon);

}

int handle_file_begin(char* name)
{
    printf("handle_file_begin(%s)\n", name);
    int fd = cfs_open(name, CFS_WRITE);
    if (fd == -1)
    {
        printf("Open file %s failed\n", name);
    }

    return fd;
}

int handle_file_data(int fd, uint8_t* data, uint8_t size)
{
    printf("handle_file_end(%x, data, %d)\n", fd, size);
    if (fd != -1)
    {
        return cfs_write(fd, data, size);
    }

    return 0;
}

void handle_file_end(int fd)
{
    printf("handle_file_end(%x)\n", fd);
    cfs_close(fd);
}

void handle_get_file(char* name)
{
    printf("handle_get_file(%s)\n", name);
    transfer_file(name);
}

static char* filter_filename_by_prefix(char* prefix, char* filename)
{
    char* pp = prefix;
    char* pf = filename;
    while (*pp != '\0' && *pf != '\0')
    {
        if (*pp != *pf)
        {
            return 0;
        }
        ++pp;
        ++pf;
    }
    if (*pf != '\0')
        return pf;
    else
        return 0;
}

//TODO: help verify
void handle_list_file(char* prefix)
{
    printf("handle_list_file(%s)\n", prefix);

    char buf[200] = "";
    uint8_t buf_size = 0;
    struct cfs_dir dir;
    int ret = cfs_opendir(&dir, "");
    if (ret == -1)
    {
        printf("cfs_opendir() failed: %d\n", ret);
        return;
    }

    while (ret != -1)
    {
        struct cfs_dirent dirent;
        ret = cfs_readdir(&dir, &dirent);
        if (ret != -1)
        {
            printf("file:%s, %d\n", dirent.name, dirent.size);

            char* short_name = filter_filename_by_prefix(prefix, dirent.name);
            uint8_t len = strlen(short_name) + 1;
            if (buf_size + len >= sizeof(buf) - 1)
                break;
            strcat(buf, short_name);
            strcat(buf, ";");
            buf_size += len;
        }
    }

    cfs_closedir(&dir);

    send_file_list(buf);

}

void handle_remove_file(char* name)
{
    printf("handle_remove_file(%s)\n", name);
    cfs_remove(name);
}

void handle_get_device_id()
{
    //TODO: return device id
    printf("handle_get_device_id()\n");
}

void handle_gps_info(uint16_t spd, uint16_t alt, uint32_t distance)
{
    printf("handle_gps_info(%d, %d, %d)\n", spd, alt, distance);
    uint32_t lspd = (spd == 0xffff ? 0xffffffff : spd);
    uint32_t lalt = (alt == 0xffff ? 0xffffffff : alt);

    window_postmessage(EVENT_SPORT_DATA, SPORTS_SPEED,    (void*)lspd);
    window_postmessage(EVENT_SPORT_DATA, SPORTS_ALT,      (void*)lalt);
    window_postmessage(EVENT_SPORT_DATA, SPORTS_DISTANCE, (void*)distance);
}

#define MAX_FILE_NAME_SIZE 32 + 1
typedef struct _file_reader_t
{
    int      read_fd;
    int      send_fd;
    uint16_t read_cursor;
    uint16_t file_size;
    char     file_name[MAX_FILE_NAME_SIZE];
}file_reader_t;

static file_reader_t _f_reader;
static void init_file_reader(file_reader_t* reader)
{
    reader->send_fd      = 0;
    reader->read_fd      = 0;
    reader->read_cursor  = 0;
    reader->file_size    = 0;
    reader->file_name[0] = '\0';
}

static void send_file_block(int para);
static void send_file_callback(int para);

static void send_file_block(int para)
{
    uint8_t buf[200];
    cfs_seek(_f_reader.read_fd, _f_reader.read_cursor, CFS_SEEK_SET);
    uint16_t byte_read = cfs_read(_f_reader.read_fd, buf, sizeof(buf));
    send_file_data(para, buf, byte_read, send_file_callback, para);
    _f_reader.read_cursor += byte_read;

}

static void send_file_callback(int para)
{
    if (_f_reader.read_cursor >= _f_reader.file_size)
    {
        end_send_file(_f_reader.send_fd);
        cfs_close(_f_reader.read_fd);
        return;
    }

    send_file_block(para);
}

int transfer_file(char* filename)
{
    int fd_read = cfs_open(filename, CFS_READ);
    if (fd_read == -1)
        return -1;

    cfs_offset_t pos = cfs_seek(fd_read, 0, CFS_SEEK_END);
    if (pos == -1)
        return -1;

    init_file_reader(&_f_reader);
    strcpy(_f_reader.file_name, filename);
    _f_reader.send_fd = begin_send_file(filename);
    _f_reader.read_fd = fd_read;
    _f_reader.file_size = (uint16_t)pos;

    send_file_block(_f_reader.send_fd);

    return 0;
}

void handle_get_sports_data(uint16_t *data, uint8_t numofdata)
{
    stlv_packet p = create_packet();
    if (p == NULL)
        return;
    element_handle h = append_element(p, NULL, "A", 1);

    element_append_data(p, h, (uint8_t*)&data, sizeof(uint16_t) * numofdata);
    send_packet(p, 0, 0);
}

void handle_get_sports_grid()
{
    stlv_packet p = create_packet();
    if (p == NULL)
        return;
    element_handle h = append_element(p, NULL, "R", 1);
    ui_config* config = window_readconfig();
    element_append_data(p, h, (uint8_t*)&config->sports_grid_data, sizeof(config->sports_grid_data));
    printf("send_sports_grid(%d)\n", sizeof(config->sports_grid_data));
    send_packet(p, 0, 0);
}

void handle_alarm(alarm_conf_t* para)
{
    printf("set alarm:\n");
    printf("  id           = %d\n", para->id);
    printf("  mode         = %d\n", para->mode);
    printf("  day_of_month = %d\n", para->day_of_month);
    printf("  day_of_week  = %d\n", para->day_of_week);
    printf("  hour         = %d\n", para->hour);
    printf("  minute       = %d\n", para->minute);

    switch(para->mode)
    {
        case ALARM_MODE_DISABLE:
        case ALARM_MODE_NO_EXIST:
            rtc_setalarm(0, 0, 0, 0);
            break;
        case ALARM_MODE_MONTHLY:
            rtc_setalarm(para->day_of_month | 0x80, 0, para->hour | 0x80, para->minute | 0x80);
            break;
        case ALARM_MODE_HOURLY:
            rtc_setalarm(0, 0, 0, para->minute | 0x80);
            break;
        case ALARM_MODE_DAILY:
            rtc_setalarm(0, 0, para->hour | 0x80, para->minute | 0x80);
            break;
        case ALARM_MODE_WEEKLY:
            rtc_setalarm(0, para->day_of_week | 0x80, para->hour | 0x80, para->minute | 0x80);
            break;
        case ALARM_MODE_ONCE:
            // no way
            break;
    }
}

void handle_gesture_control(uint8_t flag, uint8_t action_map[])
{
    ui_config* online_config = window_readconfig();
    if (online_config != NULL)
    {
        online_config->gesture_flag = flag;
        for (uint8_t i = 0; i < 4; ++i)
            online_config->gesture_map[i] = action_map[i];
        window_writeconfig();
        window_loadconfig();
    }
}

void handle_set_watch_config(ui_config* config)
{
    //TODO: help check this

    //adjust values: big endian to little endian
    config->goal_steps    = htons(config->goal_steps);
    config->goal_distance = htons(config->goal_distance);
    config->goal_calories = htons(config->goal_calories);
    config->lap_length    = htons(config->lap_length);

    if (config->weight < 20) config->weight = 20;
    if (config->height < 60) config->height = 60;

    printf("set_watch_config:\n");
    printf("  signature     = %d\n", config->signature);
    printf("  default_clock = %d\n", config->default_clock); // 0 - analog, 1 - digit
    printf("  analog_clock  = %d\n", config->analog_clock);  // num : which lock face
    printf("  digit_clock   = %d\n", config->digit_clock);   // num : which clock face
    printf("  sports_grid   = %d\n", config->sports_grid);   // 0 - 3 grid, 1 - 4 grid, 2 - 5 grid
    printf("  sports_grids  = %d, %d, %d, %d, %d\n",
            config->sports_grid_data[0],
            config->sports_grid_data[1],
            config->sports_grid_data[2],
            config->sports_grid_data[3],
            config->sports_grid_data[4]);
    printf("  goal_steps    = %d\n", config->goal_steps);
    printf("  goal_distance = %d\n", config->goal_distance);
    printf("  goal_calories = %d\n", config->goal_calories);
    printf("  weight        = %d\n", config->weight); // in kg
    printf("  height        = %d\n", config->height); // in cm
    printf("  is_ukuint     = %d\n", config->is_ukuint);
    printf("  lap_len       = %d\n", config->lap_length);
    printf("  circumference = %d\n", config->circumference);


    ui_config* online_config = window_readconfig();
    if (online_config != NULL)
    {
        memcpy(online_config, config, sizeof(ui_config));
        window_writeconfig();
        window_loadconfig();
    }
}

