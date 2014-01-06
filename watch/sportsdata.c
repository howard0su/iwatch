
#include <stdio.h>

#include "sportsdata.h"

#include <cfs/cfs.h>
#include "window.h"
#include "btstack/include/btstack/utils.h"
#include "rtc.h"

static const uint32_t signature = 0xEFAB1CC3;

typedef struct _data_desc_t
{
    const char*   file_prefix;
    const uint8_t col_count;
    const uint8_t col_desc[5];
}data_desc_t;

typedef struct _data_file_t
{
    int           file_id;
    uint16_t      row_cursor;
}data_file_t;

/*
 * data here stands for data collected within the time interval
 *  from last row to this one
 *  no Speed data here is due to avg speed can be calculated by distance / timeinterval
 * normal  : time_offset, steps, cals, distance
 * running : time_offset, steps, cals, distance, alt, heartrate
 * biking  : time_offset, cads,  cals, distance, alt, heartrate*/
/*  Columns are defined as below
    SPORTS_TIME = 0,
    SPORTS_SPEED_MAX,
    SPORTS_TIME_LAST_GPS,

    SPORTS_SPEED,
    SPORTS_ALT,
    SPORTS_DISTANCE,
    SPORTS_ALT_START,

    SPORTS_HEARTRATE,
    SPORTS_HEARTRATE_AVG,
    SPORTS_HEARTRATE_MAX,
    SPORTS_TIME_LAST_HEARTRATE,

    SPORTS_CADENCE,
    SPORTS_TIME_LAP_BEGIN,
    SPORTS_LAP_BEST,

    SPORTS_STEPS,
    SPORTS_PED_SPEED,
    SPORTS_PED_DISTANCE,
    SPORTS_PED_CALORIES,
    SPORTS_PED_STEPS_START,
    SPORTS_TIME_LAST_PED,

    SPORTS_CALS,
*/
static const data_desc_t s_data_desc[] = {
    {"w", 3, {SPORTS_TIME, SPORTS_STEPS,   SPORTS_CALS, SPORTS_DISTANCE, SPORTS_INVALID},  },
    {"r", 4, {SPORTS_TIME, SPORTS_STEPS,   SPORTS_CALS, SPORTS_DISTANCE, SPORTS_HEARTRATE},},
    {"b", 4, {SPORTS_TIME, SPORTS_CADENCE, SPORTS_CALS, SPORTS_DISTANCE, SPORTS_HEARTRATE},},
};

static data_file_t s_file_map[count_elem(s_data_desc)];
static uint8_t s_file_map_status = 0;

static void init_file_map()
{
    for (uint8_t i = 0; i < count_elem(s_file_map); ++i)
    {
        s_file_map[i].file_id    = -1;
        s_file_map[i].row_cursor = 0;
    }
}

#define get_data_file_by_mode(mode) ((mode) < count_elem(s_file_map) ? &s_file_map[(mode)] : NULL)
#define get_data_desc_by_mode(mode) ((mode) < count_elem(s_data_desc) ? &s_data_desc[(mode)] : NULL)

static void write_file_head(const data_desc_t* desc, data_file_t* f)
{
    cfs_write(f->file_id, &signature, sizeof(signature));
    cfs_write(f->file_id, &desc->col_count, sizeof(desc->col_count));

    for (uint8_t i = 0; i < desc->col_count; ++i)
        cfs_write(f->file_id, &desc->col_desc[i],  sizeof(desc->col_desc[i]));
}

static int create_data_file(uint8_t mode, const data_desc_t* desc, data_file_t* file, uint32_t timestamp, uint8_t iscontinue)
{

    uint8_t year, month, day, hour, min, sec;
    parse_timestamp(timestamp, &year, &month, &day, &hour, &min, &sec);

    char filename[32] = "";
    if (desc->file_prefix== DATA_MODE_NORMAL)
    {
        sprintf(filename, "/%s/%02d%02d%02d000000%01d",
                desc->file_prefix,
                year, month, day,
                iscontinue);
    }
    else
    {
        sprintf(filename, "/%s/%02d%02d%02d%02d%02d%02d%01d",
                desc->file_prefix,
                year, month, day, hour, min, sec,
                iscontinue);
    }

    printf("create_data_file(%s(%d))\n", filename, timestamp);

    cfs_remove(filename);

    int fd = cfs_open(filename,  CFS_WRITE | CFS_APPEND);
    if (fd == -1)
    {
        printf("Error to open a new file to write\n");
        return fd;
    }

    file->file_id    = fd;
    file->row_cursor = 0;

    write_file_head(desc, file);
    return fd;
}

void save_data_start(uint8_t mode, uint32_t timestamp)
{
    const data_desc_t* desc = get_data_desc_by_mode(mode);
    data_file_t* file = get_data_file_by_mode(mode);
    if (desc == NULL || file == NULL)
    {
        printf("save_data_start(%d) error: wrong mode\n", mode);
        return;
    }

    if (file->file_id != -1)
    {
        cfs_close(file->file_id);
        file->file_id    = -1;
        file->row_cursor = 0;

        if (create_data_file(mode, desc, file, timestamp, 0 /* continue */) == -1)
            return;
    }
}

void save_data_end(uint8_t mode)
{
    const data_desc_t* desc = get_data_desc_by_mode(mode);
    data_file_t* file = get_data_file_by_mode(mode);
    if (desc == NULL || file == NULL)
    {
        printf("save_data_end(%d) error: wrong mode\n", mode);
        return;
    }

    if (file->file_id != -1)
    {
        cfs_close(file->file_id);
        file->file_id    = -1;
        file->row_cursor = 0;
    }
}

void save_data(uint8_t mode, uint32_t timestamp, uint32_t data[], uint8_t size)
{
    const data_desc_t* desc = get_data_desc_by_mode(mode);
    data_file_t* file = get_data_file_by_mode(mode);
    if (desc == NULL || file == NULL)
    {
        printf("save_data(%d) error: wrong mode\n", mode);
        return;
    }

    if (s_file_map_status == 0)
    {
        init_file_map();
        s_file_map_status = 1;
    }

    printf("save_data(fd=%d, row=%d)\n", file->file_id, file->row_cursor);
    if (file->file_id == -1 || file->row_cursor >= MAX_ROW_COUNT)
    {
        uint8_t iscontinue = file->row_cursor >= MAX_ROW_COUNT ? 0x01 : 0x00;
        if (create_data_file(mode, desc, file, timestamp, iscontinue /* continue */) == -1)
        {
            printf("create data file failed\n");
            return;
        }
    }

    cfs_write(file->file_id, &timestamp, sizeof(timestamp));
    cfs_write(file->file_id, data, size * sizeof(data[0]));
    file->row_cursor++;
}

void write_rows(activity_raw_t row[], uint8_t size)
{
    char filename[32] = "_sportsdata";
    int fd = cfs_open(filename, CFS_WRITE | CFS_APPEND);
    if (fd == -1)
    {
        printf("cfs_open(%s) failed\n", filename);
        return;
    }

    for (uint8_t i = 0; i < size; ++i)
    {
        if (row[i].mode != DATA_MODE_TOOMSTONE)
        {
            int bytewritten = cfs_write(fd, &row[i], sizeof(row[i]));
            printf("write_data() %d/%d\n", bytewritten, sizeof(row[i]));
        }
    }

    cfs_close(fd);
}

void save_activity(uint8_t mode, uint32_t data[], uint8_t size)
{
    uint16_t year  = 0;
    uint8_t  month = 0;
    uint8_t  day   = 0, wday = 0;
    rtc_readdate(&year, &month, &day, &wday);

    uint8_t hour = 0, min = 0, sec = 0;
    if (mode != DATA_MODE_NORMAL)
        rtc_readtime(&hour, &min, &sec);

    activity_raw_t row;
    row.year      = year & 0xff;
    row.month     = month;
    row.day       = day;
    row.hour      = hour;
    row.minute    = min;
    row.second    = sec;
    row.mode      = mode;
    row.signature = 0xfe;

    for (uint8_t i = 0; i < size; ++i)
        row.data[i] = data[i];

    printf("save_activity(%02d/%0dd/%02d-%02d)\n", row.year, row.month, row.day, row.second);
    write_rows(&row, 1);
}

uint8_t load_history(activity_raw_t row[], uint8_t size)
{
    int fd = cfs_open("_sportsdata", CFS_READ);
    if (fd == -1)
    {
        printf("cfs_open(sportsdata, read) failed\n");
        return 0;
    }

    int pos = cfs_seek(fd, 0, CFS_SEEK_END);
    if (pos == -1)
    {
        printf("cfs_seek(end) failed\n");
        return 0;
    }

    int filesize = pos;
    if (pos > (int)sizeof(activity_raw_t) * size)
        pos = - sizeof(activity_raw_t) * size;

    int newpos = cfs_seek(fd, pos, CFS_SEEK_END);
    if (newpos == -1)
    {
        printf("cfs_seek(%d) failed\n", pos);
        return 0;
    }

    uint8_t i = 0;
    for (; i < size; ++i)
    {
        int byteread = cfs_read(fd, &row[i], sizeof(row[0]));
        if (byteread != sizeof(row[0]))
        {
            printf("cfs_read() failed: %d/%d\n", byteread, sizeof(row[0]));
            break;
        }
    }

    cfs_close(fd);

    if (i == size && filesize > size * 4 * sizeof(row[0]))
    {
        //write back if the file is too big
        printf("GC sportsdata:%d/%d\n", i, filesize);
        cfs_remove("_sportsdata");
        write_rows(row, i);
    }

    return i;
}

static struct cfs_dir s_sports_dir;
static uint8_t s_sports_dir_flag = 0;
static struct cfs_dirent dirent;
char* get_first_record(uint8_t mode)
{
    printf("get_history\n");
    const data_desc_t* desc = get_data_desc_by_mode(mode);
    if (desc == NULL)
    {
        printf("get_first_record(%d) error: wrong mode\n", mode);
        return NULL;
    }

    if (s_sports_dir_flag == 1)
    {
        cfs_closedir(&s_sports_dir);
    }

    int ret = cfs_opendir(&s_sports_dir, "");
    if (ret == -1)
    {
        printf("cfs_opendir() failed: %d\n", ret);
        s_sports_dir_flag = 0;
        return NULL;
    }
    s_sports_dir_flag = 1;

    ret = cfs_readdir(&s_sports_dir, &dirent);
    if (ret == -1)
    {
        cfs_closedir(&s_sports_dir);
        s_sports_dir_flag = 0;
        return NULL;
    }

    printf("get_history return %s\n", dirent.name);
    return dirent.name;
}

char* get_next_record()
{
    if (s_sports_dir_flag == 0)
        return NULL;

    int ret = cfs_readdir(&s_sports_dir, &dirent);
    if (ret == -1)
    {
        cfs_closedir(&s_sports_dir);
        s_sports_dir_flag = 0;
        return NULL;
    }

    return dirent.name;
}

uint8_t get_record_desc(char* filename, record_desc_t* record)
{
    uint8_t flag = 0;
    for (uint8_t i = 0; filename[i] != '\0'; ++i)
    {
        if (filename[i] == '/')
        {
            switch (flag)
            {
                case 0: break;
                case 2: break;
                default:
                    return 0;
                    break;
            }
        }
        else if (filename[i] == 'w' && flag == 1)
        {
            record->mode = DATA_MODE_NORMAL;
        }
        else if (filename[i] == 'r' && flag == 1)
        {
            record->mode = DATA_MODE_RUNNING;
        }
        else if (filename[i] == 'b' && flag == 1)
        {
            record->mode = DATA_MODE_BIKING;
        }
        else if (filename[i] >= '0' && filename[i] <= '9')
        {
            switch (flag)
            {
                case 3: record->year = filename[i] - '0'; break;
                case 4: record->year = record->year * 10 + filename[i] - '0'; break;

                case 5: record->month = filename[i] - '0'; break;
                case 6: record->month = record->month * 10 + filename[i] - '0'; break;

                case 7: record->day = filename[i] - '0'; break;
                case 8: record->day = record->day * 10 + filename[i] - '0'; break;

                case 9: record->hour = filename[i] - '0'; break;
                case 10: record->hour = record->hour * 10 + filename[i] - '0'; break;

                case 11: record->min = filename[i] - '0'; break;
                case 12: record->min = record->min * 10 + filename[i] - '0'; break;

                case 13: record->sec = filename[i] - '0'; break;
                case 14: record->sec = record->sec * 10 + filename[i] - '0'; break;

                case 15: record->is_continue = filename[i] - '0'; break;
                default:
                    return 0;
                    break;
            }
        }
        else
        {
            return 0;
        }

        ++flag;
    }
    return 1;
}

