#include "contiki.h"
#include "window.h"
#include "Template_Driver.h"
#include "ant/ant.h"
#include "stlv_client.h"
#include "ble_handler.h"
#include "cfs/cfs.h"
#include "memory.h"
#include "rtc.h"
#include <stdio.h> // for sprintf


//#define record_name d.recordoperation.record_name
//#define filebuf     d.recordoperation.filebuf
//#define fd_read     d.recordoperation.fd_read
//#define fd_send     d.recordoperation.fd_send
#define rows         d.menu.rows
#define row_count    d.menu.row_count

static uint8_t select_row = 0;

#define LINEMARGIN 25
static void drawItem(tContext *pContext, uint8_t n, char icon, const char* text, const char* value)
{
  if (icon)
    {
      GrContextFontSet(pContext, (tFont*)&g_sFontExIcon16);
      GrStringDraw(pContext, &icon, 1, 8, 30 + n * LINEMARGIN, 0);
    }

  // draw text
  GrContextFontSet(pContext, &g_sFontNova13);
  GrStringDraw(pContext, text, -1, 30, 30 + n * LINEMARGIN, 0);

  uint8_t width = GrStringWidthGet(pContext, value, -1);
  GrStringDraw(pContext, value, -1, LCD_X_SIZE - width - 8, 30 + n * LINEMARGIN, 0);
}

static void onDraw(tContext *pContext)
{
  char buf[30];
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);
  GrContextFontSet(pContext, &g_sFontNova38);

  activity_raw_t* row = &rows[select_row];

  if (row->mode == DATA_MODE_NORMAL || row->mode == DATA_MODE_RUNNING)
  {
    sprintf(buf, "%d", row->data[0]);
    drawItem(pContext, 0, 'y', "Steps Taken", buf);

    sprintf(buf, "%dkcal", row->data[1]);
    drawItem(pContext, 1, 'z'+1, "Calorie", buf);

    sprintf(buf, "%dm", row->data[2] / 100);
    drawItem(pContext, 2, 'z'+2, "Distance", buf);
  }
  else
  {
    sprintf(buf, "%d", row->data[0]);
    drawItem(pContext, 0, 'y', "Cadence Taken", buf);

    sprintf(buf, "%dkcal", row->data[1]);
    drawItem(pContext, 1, 'z'+1, "Calorie", buf);

    sprintf(buf, "%dm", row->data[2] / 100);
    drawItem(pContext, 2, 'z'+2, "Distance", buf);
  }

  window_button(pContext, KEY_ENTER, "SYNC");
  window_button(pContext, KEY_DOWN,  "DELETE");
}

#if 0
static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  // display the text
  //if (record_name != NULL)
  //  GrStringDraw(pContext, record_name, -1, 10, 80, 0);

  window_button(pContext, KEY_ENTER, "SYNC");
  //window_button(pContext, KEY_DOWN,  "DELETE");
}
#endif


//int begin_send_file(char* name);
//int send_file_data(int fd, uint8_t* data, uint8_t size, void (*callback)(int), int para);
//void end_send_file(int fd);
//static void sendfile_callbak(int para)
//{
//   process_post(ui_process, EVENT_STLV_DATA_SENT, NULL);
//}
//
//static void send_record_proc()
//{
//    int size = cfs_read(fd_read, filebuf, sizeof(filebuf));
//    if (size > 0)
//    {
//        send_file_data(fd_send, filebuf, size, sendfile_callbak, fd_read);
//        printf("record send %d bytes\n", size);
//    }
//    else
//    {
//        end_send_file(fd_send);
//        cfs_close(fd_read);
//        printf("record send end\n");
//    }
//}

uint8_t recordoperation_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
    //record_name = (char*)rparam;
    select_row = (uint8_t)rparam;
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
    switch (lparam)
    {
        case KEY_ENTER:
            {
                activity_raw_t* row = &rows[select_row];

                //BLE
                uint32_t timestamp = calc_timestamp(row->year, row->month, row->day, row->hour, row->minute, row->second);
                ble_start_sync(SPORTS_SYNC_MODE_SYNC);
                ble_send_sports_data(timestamp, &row->data[0], 4);

                //STLV
                uint32_t data[4] = {0};
                data[0] = timestamp;
                data[1] = row->data[0];
                data[2] = row->data[1];
                data[3] = row->data[2];
                send_sports_data(0, SPORTS_DATA_FLAG_SYNC, &row->data[0], 4);
            }
        case KEY_DOWN:
            {
                cfs_remove("_sportsdata");
                rows[select_row].mode = DATA_MODE_TOOMSTONE;
                write_rows(rows, row_count);
            }

            break;
    }
    window_close();

    //if (lparam == KEY_ENTER)
    //{
        //fd_read = cfs_open(record_name, CFS_READ);
        //if (fd_read  == -1) {
        //    printf("cfs_open(%s, CFS_READ) failed\n", record_name);
        //}
        //else {
        //    fd_send = begin_send_file(record_name);
        //    if (fd_send == -1)
        //    {
        //        printf("cfs_open(%s, CFS_READ) failed\n", record_name);
        //        cfs_close(fd_read);
        //    }
        //    else
        //    {
        //        send_record_proc();
        //    }
        //}

    //}
    //else if (lparam == KEY_DOWN)
    //{
    //    cfs_remove(record_name);
    //}
    //window_close();
    break;

  case EVENT_STLV_DATA_SENT:
    break;

  default:
    return 0;
  }

  return 1;
}
