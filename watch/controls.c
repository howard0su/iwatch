#include "contiki.h"

#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"


void window_drawtime(tContext *pContext, long y, uint8_t times[3], uint8_t selected)
{
  char data[3];
  data[0] = data[1] = '0';
  data[2] = ':';
  #define SPACING 3
  uint8_t height = GrStringHeightGet(pContext);
  uint8_t width_all = GrStringWidthGet(pContext, data, 3);
  uint8_t width_digit = GrStringWidthGet(pContext, data, 2);

  long startx = (LCD_X_SIZE - width_all - width_all - width_digit) / 2;
  if (startx < 0) startx = 0;

  for(int i = 0; i < 3; i++)
  {
    data[0] = '0' + times[i] / 10;
    data[1] = '0' + times[i] % 10;

    if (selected & (1 << i))
    {
      // revert color
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextBackgroundSet(pContext, ClrBlack);

      tRectangle rect = {startx + i * width_all, y, startx + i * width_all + width_digit, y + height};
      GrRectFill(pContext, &rect);
      GrContextForegroundSet(pContext, ClrBlack);
      GrContextBackgroundSet(pContext, ClrWhite);
    }
    else
    {
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextBackgroundSet(pContext, ClrBlack);
    }

    GrStringDraw(pContext, data, 2, startx + SPACING + i * width_all, y, 0);

    if (i != 2)
    {
      GrContextForegroundSet(pContext, ClrWhite);
      GrContextBackgroundSet(pContext, ClrBlack);
      GrStringDraw(pContext, ":", 1, startx + SPACING + width_digit + i * width_all, y, 0);
    }
  }
}

void window_progress(tContext *pContext, long lY, uint8_t step)
{
  tRectangle rect = {20, lY, 125, lY + 16};
  GrContextForegroundSet(pContext, ClrWhite);
  GrRectFill(pContext, &rect);
  GrContextForegroundSet(pContext, ClrBlack);

  if (step < 100)
  {
    rect.sXMin = 22;
    rect.sYMin = lY + 2;
    rect.sYMax = lY + 14;
    rect.sXMax = 22 + step;
    GrRectFill(pContext, &rect);
  }
}

/*
* Draw the button text for the keys
* If text is NULL, draw a empty box
*/
void window_button(tContext *pContext, uint8_t key, const char* text)
{
#define SPACE 2
  uint8_t width, height;
  int x, y;

  GrContextFontSet(pContext, &g_sFontRed13);
  if (!text)
  {
    width = 100;
  }
  else
  {
    width = GrStringWidthGet(pContext, text, -1);
  }

  height = GrStringHeightGet(pContext);

  if (key == KEY_UP || key == KEY_DOWN)
  {
    x = LCD_X_SIZE - width - SPACE;
  }
  else
  {
    x = SPACE;
  }

  if (key == KEY_ENTER || key == KEY_DOWN)
  {
    y = 135;
  }
  else
  {
    y = 30;
  }

  // draw black box
  const tRectangle rect = {x - SPACE, y - SPACE, x + width + SPACE, y + height + SPACE};

  if (text)
  {
    GrContextForegroundSet(pContext, ClrWhite);
    GrRectFillRound(pContext, &rect, 2);
    GrContextForegroundSet(pContext, ClrBlack);
    GrStringDraw(pContext, text, -1, x, y + 1, 0);
  }
  else
  {
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFillRound(pContext, &rect, 2);
  }

  GrContextForegroundSet(pContext, ClrWhite);
#undef SPACE
}