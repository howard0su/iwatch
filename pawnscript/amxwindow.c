
#include "osdefs.h"
#include "amx.h"

#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "rtc.h"

static const tFont* fonts[] =
{
&g_sFontNova12b,
&g_sFontNova13,
&g_sFontNova16,
&g_sFontNova16b,
&g_sFontNova28,
&g_sFontNova28b,
&g_sFontNova38,
&g_sFontNova38b,
&g_sFontNova50b,
&g_sFontRed13,
&g_sFontBaby16,
&g_sFontBaby12,
(tFont*)&g_sFontExIcon16,
(tFont*)&g_sFontExIcon48,
(tFont*)&g_sFontExDigit44,
(tFont*)&g_sFontExDigit44b,
(tFont*)&g_sFontExDigit56,
(tFont*)&g_sFontExDigit52b,
};

static cell AMX_NATIVE_CALL n_invalid(AMX *amx,const cell *params)
{
  window_invalid(NULL);

  return 0;
}

static cell AMX_NATIVE_CALL n_invalid_rect(AMX *amx,const cell *params)
{
  tRectangle rect;
  rect.sXMin = params[1];
  rect.sYMin = params[2];
  rect.sXMax = params[1] + params[3];
  rect.sYMax = params[2] + params[4];

  window_invalid(&rect);

  return 0;
}

static cell AMX_NATIVE_CALL n_setfont(AMX *amx, const cell *params)
{
  tContext *context = window_context();
  uint8_t font = (uint8_t)params[2];

  if (font < sizeof(fonts))
    GrContextFontSet(context, fonts[font]);

  return 0;
}

//native window_getwidth(context, string[])
static cell AMX_NATIVE_CALL n_getwidth(AMX *amx, const cell *params)
{
  tContext *context = window_context();
  char *text;

  amx_StrParam(amx, params[2], text);

  return (cell)GrStringWidthGet(context, text, -1);
}

// window_drawtext(context, string[], x, y, style)
static cell AMX_NATIVE_CALL n_drawtext(AMX *amx, const cell *params)
{
  tContext *context = window_context();
  char *text;

  amx_StrParam(amx, params[2], text);

  GrStringDraw(context, text, -1, params[3], params[4], params[5]);

  return 0;
}

static cell AMX_NATIVE_CALL n_gettime(AMX *amx, const cell *params)
{
  cell *cptr;

  printf("n_gettime called\n");
  assert(params[0]==(int)(3*sizeof(cell)));

  uint8_t hour, minute, second;
  rtc_readtime(&hour, &minute, &second);

  cptr=amx_Address(amx,params[1]);
  *cptr=hour;
  cptr=amx_Address(amx,params[2]);
  *cptr=minute;
  cptr=amx_Address(amx,params[3]);
  *cptr=second;

  return 0;
}

static cell AMX_NATIVE_CALL n_getdate(AMX *amx, const cell *params)
{
  cell *cptr;

  printf("n_getdate called\n");
  assert(params[0]==(int)(3*sizeof(cell)));

  uint16_t year;
  uint8_t month, day;
  rtc_readdate(&year, &month, &day, NULL);

  cptr=amx_Address(amx,params[1]);
  *cptr=year;
  cptr=amx_Address(amx,params[2]);
  *cptr=month;
  cptr=amx_Address(amx,params[3]);
  *cptr=day;

  return 0;
}

static cell AMX_NATIVE_CALL n_enableclock(AMX *amx, const cell *params)
{
  printf("n_enableclock %x called\n", params[1]);
  rtc_enablechange(params[1]);

  return 0;
}

extern cell AMX_NATIVE_CALL n_strformat(AMX *amx,const cell *params);

AMX_NATIVE const window_natives[] =
{
  n_invalid,
  n_invalid_rect,
  n_setfont,
  n_getwidth,
  n_drawtext,
  n_strformat,
  n_gettime,
  n_getdate,
  n_enableclock
};
