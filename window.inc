#pragma library Window

const {
   LCD_WIDTH = 144,
   LCD_HEIGHT = 168
}

const clocktype: {
  Second = 1,
  Minute = 2,
  TenMS = 4
}

const color:
{
  White = 0,
  Black = 65535
}

const fonts: {
  FontNova12b = 0,
  FontNova13,
  FontNova16,
  FontNova16b,
  FontNova28,
  FontNova28b,
  FontNova38,
  FontNova38b,
  FontNova50b,
  FontRed13,
  FontBaby16,
  FontBaby12,
  FontIcon16,
  FontIcon48,
  FontDigit44,
  FontDigit44b,
  FontDigit56,
  FontDigit52b,
}

forward @oncreate();
forward @onclock();
forward @onpaint(context: context);
forward @onkey(key);
forward @onclose();

native window_invalid() = -1
native window_invalid_rect(x, y, w, h) = -2
native window_setfont(context: context, fonts: font) = -3
native window_getwidth(context: context, string[]) = -4
native window_drawtext(context: context, string[], x, y, style) = -5
native window_drawtext_center(context, string[], x, y, style) = -10
native window_filltriagle(context: context, xa, ya, xb, yb, xc, yc) = -11
native window_setcolor(context: context, fg : color, bg : color) = -12
//native window_drawrect(x, y, w, h)

native strformat(dest[], size=sizeof dest, bool:pack=true, const format[], {Fixed,Float,_}:...) = -6;

native gettime(&hour=0, &minute=0, &second=0) = -7
native getdate(&year=0, &month=0, &day=0) = -8
//native settime(hour=cellmin, minute=cellmin, second=cellmin) = -9
//native setdate(year=cellmin, month=cellmin, day=cellmin) = -10
native enableclock(clocktype: t) = -9
