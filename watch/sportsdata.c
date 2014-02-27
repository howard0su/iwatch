
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
 * normal  : steps, cals, distance,
 * running : steps, cals, distance, alt, heartrate
 * biking  : cads,  cals, distance, alt, heartrate*/
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
static const data_desc_t s_data_desc[] = {
    {"w", 3, {SPORTS_TIME, SPORTS_STEPS,   SPORTS_CALS, SPORTS_DISTANCE, SPORTS_INVALID},  },
    {"r", 4, {SPORTS_TIME, SPORTS_STEPS,   SPORTS_CALS, SPORTS_DISTANCE, SPORTS_HEARTRATE},},
    {"b", 4, {SPORTS_TIME, SPORTS_CADENCE, SPORTS_CALS, SPORTS_DISTANCE, SPORTS_HEARTRATE},},
};

*/
typedef struct _data_head_t
{
    uint8_t version;
    uint8_t year;
    uint8_t month;
    uint8_t day;
}data_head_t;

static void write_file_head(int fd, uint8_t year, uint8_t month, uint8_t day)
{
    cfs_write(fd, &signature, sizeof(signature));

    data_head_t data_head;
    data_head.version = 1;
    data_head.year    = 1;
    data_head.month   = 1;
    data_head.day     = 1;
    cfs_write(fd, &data_head, sizeof(data_head));
}

static int s_data_fd = -1;
static const char* s_data_dir = "DATA";

int create_data_file(uint8_t year, uint8_t month, uint8_t day)
{
    char filename[32] = "";
    sprintf(filename, "/%s/%02d-%02d-%02d", s_data_dir, year, month, day);
    s_data_fd = cfs_open(filename,  CFS_WRITE | CFS_APPEND);
    if (s_data_fd == -1)
    {
        printf("create_data_file(%d, %d, %d) failed\n", year, month, day);
    }
    else
    {
        printf("create_data_file(%d, %d, %d) ok\n", year, month, day);
        write_file_head(s_data_fd, year, month, day);
    }
    return s_data_fd;
}

void write_data_line(uint8_t mode, uint8_t hh, uint8_t mm, uint8_t meta[], uint32_t data[], uint8_t size)
{
    if (s_data_fd != -1)
    {
        uint32_t tag = mode << 24 | hh << 16 | mm << 8 | size;
        if (cfs_write(s_data_fd, &tag, sizeof(tag)) != sizeof(tag))
        {
            printf("write_data(%d, %x, %d, %d, %d) failed\n", s_data_fd, mode, hh, mm, size);
            close_data_file();
            return;
        }

        uint32_t meta_tag = build_data_schema(meta, size);
        if (cfs_write(s_data_fd, &tag, sizeof(tag)) != sizeof(tag))
        {
            printf("write_data(%d, meta) failed\n", s_data_fd);
            close_data_file();
            return;
        }

        if (cfs_write(s_data_fd, data, size * sizeof(uint32_t) != size * sizeof(uint32_t)))
        {
            printf("write_data(%d, data) failed\n", s_data_fd);
            close_data_file();
            return;
        }
    }
}

void close_data_file()
{
    if (s_data_fd != -1)
    {
        cfs_close(s_data_fd);
        s_data_fd = -1;
    }
}

char* get_data_file()
{
    //for test
    struct cfs_dir dir;
    int ret = cfs_opendir(&dir, "");
    if (ret == -1)
    {
        printf("cfs_opendir() failed: %d\n", ret);
        return 0;
    }

    while (ret != -1)
    {
        static struct cfs_dirent dirent;
        ret = cfs_readdir(&dir, &dirent);
        if (ret != -1)
        {
            if (dirent.name[0] == '/' &&
                dirent.name[1] == 'D' &&
                dirent.name[1] == 'A' &&
                dirent.name[1] == 'T' &&
                dirent.name[1] == 'A' &&
                dirent.name[1] == '/')
            {
                printf("file:%s, %d\n", dirent.name, dirent.size);
                cfs_closedir(&dir);
                return dirent.name;
            }
        }
    }

    cfs_closedir(&dir);
    return 0;

}

void remove_data_file(char* filename)
{
    cfs_remove(filename);
}

uint32_t build_data_schema(uint8_t coltype[], uint8_t colcount)
{
    uint32_t ret = 0;
    for (uint8_t i = 0; i < colcount; ++i)
    {
        uint8_t val = coltype[i] & 0x0f;
        uint8_t shift = (8 - i) * 4;
        ret |= (val << shift);
    }
    return ret;
}


static uint8_t s_cur_mode = DATA_MODE_NORMAL;
uint8_t set_mode(uint8_t mode)
{
    s_cur_mode = mode;
}

uint8_t get_mode()
{
    return s_cur_mode;
}
