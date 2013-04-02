#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include <stdio.h>

struct process* ui_process = NULL;

void window_init()
{
  return;
}

tContext context;

#if 0
#define MAX_DIALOG_DEPTH 5
static struct process *dialogStack[MAX_DIALOG_DEPTH];
static uint8_t dialogStackPtr = 0;

void window_showdialog(struct process* dialog, void* data)
{
  process_start(dialog, data);
  dialogStack[dialogStackPtr++] = ui_process;
  ui_process = dialog;
  process_post(dialog, EVENT_WINDOW_CREATED, NULL);
}
#endif

void window_open(struct process* dialog, void* data)
{
  if (ui_process)
  {
    process_post(ui_process, EVENT_WINDOW_CLOSING, NULL);
    process_exit(ui_process);
  }
  process_start(dialog, data);
  ui_process = dialog;
  process_post(dialog, EVENT_WINDOW_CREATED, NULL);
}

static uint8_t bt_status;

void window_defproc(process_event_t ev, process_data_t data)
{
  switch(ev)
  {
  case EVENT_BT_STATUS:
    {
      bt_status = (uint8_t)data;
      if (bt_status & BIT0) GrStringDraw(&context, "B", 1, 90, 2, 0);
      else GrStringDraw(&context, " ", 1, 90, 2, 0);
      if (bt_status & BIT1) GrStringDraw(&context, "=", 1, 102, 2, 0);
      else GrStringDraw(&context, " ", 1, 102, 2, 0);
      GrFlush(&context);
      break;
    }
  case PROCESS_EVENT_INIT:
    {
      process_post(PROCESS_CURRENT(), EVENT_WINDOW_CREATED, data);
      process_post(PROCESS_CURRENT(), EVENT_BT_STATUS, (void*)bt_status);
      break;
    }
  case EVENT_KEY_LONGPRESSED:
    {
      if ((uint8_t)data == KEY_ENTER && ui_process != &menu_process)
      {
        window_open(&menu_process, 0);
      }

      printf("Key Long Pressed: %d\n", (uint8_t)data);
      break;
    }
  case EVENT_KEY_PRESSED:
    {
      printf("Key Pressed: %d\n", (uint8_t)data);
      break;
    }
  }
  return;
}
