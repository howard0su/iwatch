#include "contiki.h"
#include "window.h"
#include "Template_Driver.h"
#include "ant/ant.h"
#include "stlv_client.h"
#include "ble_handler.h"
#include "cfs/cfs.h"
#include "memory.h"

#define record_name d.recordoperation.record_name
#define filebuf     d.recordoperation.filebuf
#define fd_read     d.recordoperation.fd_read
#define fd_send     d.recordoperation.fd_send

static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  // display the text
  if (record_name != NULL)
    GrStringDraw(pContext, record_name, -1, 10, 80, 0);
  window_button(pContext, KEY_ENTER, "SYNC");
  window_button(pContext, KEY_DOWN,  "DELETE");
}


//int begin_send_file(char* name);
//int send_file_data(int fd, uint8_t* data, uint8_t size, void (*callback)(int), int para);
//void end_send_file(int fd);
static void sendfile_callbak(int para)
{
   process_post(ui_process, EVENT_STLV_DATA_SENT, NULL);
}

static void send_record_proc()
{
    int size = cfs_read(fd_read, filebuf, sizeof(filebuf));
    if (size > 0)
    {
        send_file_data(fd_send, filebuf, size, sendfile_callbak, fd_read);
        printf("record send %d bytes\n", size);
    }
    else
    {
        end_send_file(fd_send);
        cfs_close(fd_read);
        printf("record send end\n");
    }
}

uint8_t recordoperation_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
    record_name = (char*)rparam;
    return 0x80;
  case EVENT_WINDOW_CLOSING:
    break;
  case EVENT_WINDOW_PAINT:
    onDraw((tContext*)rparam);
    break;
  case EVENT_NOTIFY_RESULT:
    window_close();
    break;
  case EVENT_KEY_PRESSED:
    if (lparam == KEY_ENTER)
    {
        fd_read = cfs_open(record_name, CFS_READ);
        if (fd_read  == -1) {
            printf("cfs_open(%s, CFS_READ) failed\n", record_name);
        }
        else {
            fd_send = begin_send_file(record_name);
            if (fd_send == -1)
            {
                printf("cfs_open(%s, CFS_READ) failed\n", record_name);
                cfs_close(fd_read);
            }
            else
            {
                send_record_proc();
            }
        }
    }
    else if (lparam == KEY_DOWN)
    {
        cfs_remove(record_name);
    }
    window_close();
    break;

  case EVENT_STLV_DATA_SENT:
    break;

  default:
    return 0;
  }

  return 1;
}
