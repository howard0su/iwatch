#pragma library Window

const {
   LCD_WIDTH = 144,
   LCD_HEIGHT = 168
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
forward @ontimer();
forward @onpaint(context);
forward @onkey(key);
forward @onclose();

native window_invalid()
native window_invalid_rect(x, y, w, h)
native window_setfont(context, fonts: font)
native window_getwidth(context, string[])
native window_drawtext(context, string[], x, y, style)
//native window_drawrect(x, y, w, h)

