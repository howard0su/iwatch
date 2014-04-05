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

      tRectangle rect = {startx + i * width_all + 2, y, startx + i * width_all + width_digit + 1, y + height};
      GrRectFillRound(pContext, &rect, 3);
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
  tRectangle rect = {18, lY, 127, lY + 8};
  GrContextForegroundSet(pContext, ClrWhite);
  GrRectFillRound(pContext, &rect, 3);
  GrContextForegroundSet(pContext, ClrBlack);

  if (step > 100)
    step = 100;

  rect.sXMin += 4;
  rect.sYMin += 2;
  rect.sYMax -= 2;
  rect.sXMax = rect.sXMin + step;
  GrRectFill(pContext, &rect);
}

void window_volume(tContext *pContext, long lX, long lY, int total, int current)
{
  for (int i = 0; i <= total; i++)
  {
    tRectangle rect = {lX + i * 10, lY - 3 - i * 3, lX + 7 + i * 10, lY};
    if (i <= current)
    {
      // solid
      GrRectFill(pContext, &rect);
    }
    else
    {
      GrRectDraw(pContext, &rect);
    }
  }
}

static const tRectangle button_rect[] = 
{
 {11, 145, 68, 161},
 {11, 28, 68, 44},
 {77, 28, 131, 44},
 {77, 145, 131, 161},
};

/*
* Draw the button text for the keys
* If text is NULL, draw a empty box
*/
void window_button(tContext *pContext, uint8_t key, const char* text)
{
  GrContextFontSet(pContext, &g_sFontGothic18);

  // draw black box
  if (text)
  {
    const tRectangle *rect = &button_rect[key];
    GrContextForegroundSet(pContext, ClrWhite);
    GrRectFill(pContext, rect);

    // draw triagle
    for(int i = 0; i <= (rect->sYMax - rect->sYMin) /2 ; i++)
    {
      if (rect->sXMin < 20)
      {
        GrLineDrawH(pContext, rect->sXMin - i, rect->sXMin, rect->sYMin + i);
        GrLineDrawH(pContext, rect->sXMin - i, rect->sXMin, rect->sYMax - i);
      }
      else
      {
        GrLineDrawH(pContext, rect->sXMax, rect->sXMax + i, rect->sYMin + i);
        GrLineDrawH(pContext, rect->sXMax, rect->sXMax + i, rect->sYMax - i);
      }
    }

    GrContextForegroundSet(pContext, ClrBlack);
    GrStringDrawCentered(pContext, text, -1, (rect->sXMin + rect->sXMax) /2, (rect->sYMin + rect->sYMax) /2, 0);
  }
  else
  {
    GrContextForegroundSet(pContext, ClrBlack);
    GrRectFill(pContext, &button_rect[key]);
  }

  GrContextForegroundSet(pContext, ClrWhite);
}