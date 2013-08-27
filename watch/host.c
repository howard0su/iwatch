#include "contiki.h"
#include "pawnscript/amx.h"
#include "window.h"

#include <cfs/cfs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#define hexdump(...)
#endif

static AMX amx;
static int idxOnCreate, idxOnPaint, idxOnClose, idxOnClock;
static uint16_t mem[800];

static enum {RUNNING, ERROR, DONE} state;

static const char * AMXAPI aux_StrError(int errnum)
{
 static const char *messages[] = {
      /* AMX_ERR_NONE      */ "(none)",
      /* AMX_ERR_EXIT      */ "Forced exit",
      /* AMX_ERR_ASSERT    */ "Assertion failed",
      /* AMX_ERR_STACKERR  */ "Stack/heap collision (insufficient stack size)",
      /* AMX_ERR_BOUNDS    */ "Array index out of bounds",
      /* AMX_ERR_MEMACCESS */ "Invalid memory access",
      /* AMX_ERR_INVINSTR  */ "Invalid instruction",
      /* AMX_ERR_STACKLOW  */ "Stack underflow",
      /* AMX_ERR_HEAPLOW   */ "Heap underflow",
      /* AMX_ERR_CALLBACK  */ "No (valid) native function callback",
      /* AMX_ERR_NATIVE    */ "Native function failed",
      /* AMX_ERR_DIVIDE    */ "Divide by zero",
      /* AMX_ERR_SLEEP     */ "(sleep mode)",
      /* AMX_ERR_INVSTATE  */ "Invalid state",
      /* 14 */                "(reserved)",
      /* 15 */                "(reserved)",
      /* AMX_ERR_MEMORY    */ "Out of memory",
      /* AMX_ERR_FORMAT    */ "Invalid/unsupported P-code file format",
      /* AMX_ERR_VERSION   */ "File is for a newer version of the AMX",
      /* AMX_ERR_NOTFOUND  */ "File or function is not found",
      /* AMX_ERR_INDEX     */ "Invalid index parameter (bad entry point)",
      /* AMX_ERR_DEBUG     */ "Debugger cannot run",
      /* AMX_ERR_INIT      */ "AMX not initialized (or doubly initialized)",
      /* AMX_ERR_USERDATA  */ "Unable to set user data field (table full)",
      /* AMX_ERR_INIT_JIT  */ "Cannot initialize the JIT",
      /* AMX_ERR_PARAMS    */ "Parameter error",
      /* AMX_ERR_DOMAIN    */ "Domain error, expression result does not fit in range",
      /* AMX_ERR_GENERAL   */ "General error (unknown or unspecific error)",
      /* AMX_ERR_OVERLAY   */ "Overlays are unsupported (JIT) or uninitialized",
    };
  if (errnum < 0 || errnum >= sizeof messages / sizeof messages[0])
    return "(unknown)";
  return messages[errnum];
}


static int init_engine(void* mem)
{
  int error = amx_Init(&amx, mem);
  if (error != AMX_ERR_NONE)
  {
    PRINTF("Error Init: %s\n", aux_StrError(error));
    return -1;
  }
 
  amx_FindPublic(&amx, "@oncreate", &idxOnCreate);
  amx_FindPublic(&amx, "@onpaint", &idxOnPaint);
  amx_FindPublic(&amx, "@onclose", &idxOnClose);
  amx_FindPublic(&amx, "@onclock", &idxOnClock);

  return 0;
}

static int init_script(void *rom)
{
  uint16_t length;
  /* initialize the abstract machine */
  memset(&amx, 0, sizeof amx);
 
  // rom is not always aligned
  length = *((uint8_t*)rom + 1);
  length *= 256;
  length +=*(uint8_t*)rom;

  if (mem != rom)
  {
    PRINTF("copy rom size=%d\n", length);
    memcpy(mem, rom, length);
  }

  return init_engine(mem);
}

static int load_script(const char* filename)
{
  AMX_HEADER hdr;
  // try to load file from spi flash
  int fd = cfs_open(filename, CFS_READ);
  if (fd == -1)
    return AMX_ERR_NOTFOUND;

  // read the size
  cfs_read(fd, &hdr, sizeof hdr);
  if (hdr.magic != AMX_MAGIC) {
    cfs_close(fd);
    return AMX_ERR_FORMAT;
  } /* if */

  if (hdr.stp > sizeof(mem) * sizeof(mem[0]))
  {
    // no enough memory
    return AMX_ERR_MEMORY;
  }

  memcpy(mem, &hdr, sizeof hdr);

  // read the rest of file
  cfs_read(fd, (uint8_t*)mem + sizeof hdr, hdr.size - sizeof hdr);

  cfs_close(fd);

  return init_engine(mem);
}

uint8_t script_process(uint8_t event, uint16_t lparam, void* rparam)
{
  int error;
  cell ret;

  switch(event)
  {
    case EVENT_WINDOW_CREATED:
    {
      int code;
      if (*(char*)rparam == '/')
      {
        code = load_script((const char*)rparam);
      }
      else
      {
        code = init_script(rparam);
      }

      if (code != 0)
      {
        PRINTF("Error when loading %s\n", aux_StrError(code));
        state = ERROR;
        idxOnCreate = code;
        return 0;
      }

      if (idxOnCreate != INT_MAX)
      {
        PRINTF("try to run oncreate\n");
        error = amx_Exec(&amx, &ret, idxOnCreate);
        if (error != AMX_ERR_NONE)
        {
          PRINTF("Error Exec: %s\n", aux_StrError(error));
          state = ERROR;
          idxOnCreate = error;
        }
      }
      break;
    }
    case EVENT_WINDOW_PAINT:
    {
      tContext *pContext = (tContext*)rparam;
      GrContextForegroundSet(pContext, ClrBlack);
      GrRectFill(pContext, &client_clip);
      GrContextForegroundSet(pContext, ClrWhite);

      if (state == ERROR)
      {
        GrContextFontSet(pContext, &g_sFontNova16);
        GrStringDrawWrap(pContext, aux_StrError(idxOnCreate), 10, 80, LCD_X_SIZE - 10, 18);

        return 1;
      }

      if (idxOnPaint != INT_MAX)
      {
        PRINTF("try to run onpaint\n");
        amx_Push(&amx, (cell)rparam);
        error = amx_Exec(&amx, &ret, idxOnPaint);
        if (error != AMX_ERR_NONE)
        {
          PRINTF("Error Exec: %s\n", aux_StrError(error));
          state = ERROR;
          idxOnCreate = error;
          window_invalid(NULL);
        }
      }
      break;
    }
    case EVENT_TIME_CHANGED:
    if (idxOnClock != INT_MAX)
    {
      PRINTF("try to run onclock\n");
      error = amx_Exec(&amx, &ret, idxOnClock);
      if (error != AMX_ERR_NONE)
      {
        PRINTF("Error Exec: %s\n", aux_StrError(error));
        state = ERROR;
        idxOnCreate = error;
        window_invalid(NULL);
      }
    }
    break;
    case EVENT_WINDOW_CLOSING:
    {
      if (state == ERROR)
      {
        return 1;
      }
      else
      if (idxOnClose != INT_MAX)
      {
        PRINTF("try to run onclose\n");
        error = amx_Exec(&amx, &ret, idxOnClose);
        if (error != AMX_ERR_NONE)
        {
          PRINTF("Error Exec: %s\n", aux_StrError(error));
          state = ERROR;
          idxOnCreate = error;
          window_invalid(NULL);
        }
      }
      break;
    }
    default:
    return 0;
  }

  return 1;
}
