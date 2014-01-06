
#include <stdio.h>

#include "contiki.h"
#include "button.h"
#include "window.h"
#include "system.h"
#include "rtc.h"
#include "sportsdata.h"
#include "cfs/cfs.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) PRINTF(__VA_ARGs_)
#else
#define PRINTF(...)
#endif

PROCESS(filesys_process, "FileSys Proc");

void filesys_init()
{
    process_start(&filesys_process, NULL);
}

static struct cfs_dir    s_dir;
static struct cfs_dirent s_dirent;
static uint8_t s_dir_flag = 0;

PROCESS_THREAD(filesys_process, ev, data)
{
    PROCESS_BEGIN();
    while(1)
    {
        PROCESS_WAIT_EVENT();
        switch (ev)
        {
        case PROCESS_EVENT_READ_DIR:
            {
                if (s_dir_flag == 1)
                    cfs_closedir(&s_dir);

                int ret = cfs_opendir(&s_dir, "");
                if (ret == -1)
                {
                    printf("cfs_opendir() failed: %d\n", ret);
                    s_dir_flag = 0;
                    process_post(ui_process, EVENT_FILESYS_LIST_FILE, (void*)-1);
                    break;
                }
                s_dir_flag = 1;
                process_post(&filesys_process, PROCESS_EVENT_READ_DIR_PROC, NULL);
            }
            break;
        case PROCESS_EVENT_READ_DIR_PROC:
            {
                if (s_dir_flag == 0)
                    break;

                int ret = cfs_readdir(&s_dir, &s_dirent);
                if (ret == -1)
                {
                    cfs_closedir(&s_dir);
                    s_dir_flag = 0;
                    process_post(ui_process, EVENT_FILESYS_LIST_FILE, 0);
                    break;
                }

                printf("read dir:%s\n", s_dirent.name);

                process_post(ui_process, EVENT_FILESYS_LIST_FILE, s_dirent.name);
            }

            break;
        }
    }
    PROCESS_END();
}

