
#include "stlv_handler.h"

#include <stdio.h>
#include <string.h>

#include "stlv.h"
#include "contiki.h"
#include "window.h"
#include "rtc.h"
#include "cfs/cfs.h"
#include "stlv_client.h"

void handle_echo(uint8_t* data, int data_len)
{
    send_echo(data, data_len);
}

void handle_clock(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_setdate(2000 + year, month, day);
    rtc_settime(hour, minute, second);
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
    default:
        return;
        break;
    }

    window_notify(ident, message, NOTIFY_OK, icon);

}

int handle_file_begin(char* name)
{
    int fd = cfs_open(name, CFS_WRITE);
    if (fd == -1)
    {
        printf("Open file %s failed\n", name);
    }

    return fd;
}

int handle_file_data(int fd, uint8_t* data, uint8_t size)
{
    if (fd != -1)
    {
        return cfs_write(fd, data, size);
    }

    return 0;
}

void handle_file_end(int fd)
{
    cfs_close(fd);
}

//TODO: sujun implement this heart beat and get file
void handle_sports_heartbeat(uint8_t seconds_to_next)
{
    UNUSED_VAR(seconds_to_next);
}

void handle_get_file(char* name)
{
    transfer_file(name);
}

void handle_list_file()
{
}

void handle_remove_file(char* name)
{
    UNUSED_VAR(name);
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

    element_append_data(p, h, (unsigned char*)&data, sizeof(uint16_t) * numofdata);
    send_packet(p, 0, 0);
}

void handle_get_sports_grid()
{
    stlv_packet p = create_packet();
    if (p == NULL)
        return;
    element_handle h = append_element(p, NULL, "R", 1);
    ui_config* config = window_readconfig();
    element_append_data(p, h, (unsigned char*)&config->sports_grid, sizeof(uint8_t) * (config->sports_grid + 3));
    printf("send_sports_grid(%d)\n", sizeof(uint8_t) * (config->sports_grid + 3));
    send_packet(p, 0, 0);
}



