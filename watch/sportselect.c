#include "contiki.h"

#include "window.h"

static uint8_t selection;
static const char *text[2] = {"Cycling", "Running"};

static void onDraw(tContext *pContext)
{
  GrContextForegroundSet(pContext, ClrBlack);
  GrRectFill(pContext, &client_clip);

  GrContextForegroundSet(pContext, ClrWhite);

  for(int i = 0; i < 2; i++)
  {
  	char buf = 'a' + i;

	GrContextFontSet(pContext, (const tFont*)&g_sFontExBicon48);
  	if (i == selection)
  	{
  		tRectangle rect = {14, 17 + i * 84, 115, 87 + i * 84};
  		GrContextForegroundSet(pContext, ClrWhite);
		GrRectFillRound(pContext, &rect, 3);

		GrContextForegroundSet(pContext, ClrBlack);
		GrStringDraw(pContext, &buf, 1, 38, 17 + i * 84, 0);

		GrContextFontSet(pContext, &g_sFontNova12b);
		GrStringDraw(pContext, text[i], -1, 38, 67 + i * 84, 0);		
  	}
  	else
  	{
		GrContextForegroundSet(pContext, ClrWhite);
		GrStringDraw(pContext, &buf, 1, 38, 17 + i * 84, 0);
  		GrContextFontSet(pContext, &g_sFontNova12b);
		GrStringDraw(pContext, text[i], -1, 38, 67 + i * 84, 0);
  	}
  }
}

// select sport type
uint8_t sporttype_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  switch(ev){
  case EVENT_WINDOW_CREATED:
  	break;
  case EVENT_WINDOW_PAINT:
  	onDraw((tContext*)rparam);
  	break;
  case EVENT_KEY_PRESSED:
  	if (lparam == KEY_UP && selection > 0)
  		selection--;
  	if (lparam == KEY_DOWN && selection < 1)
  		selection++;
  	if (lparam == KEY_ENTER)
  	{
  		window_open(&sportswatch_process, (void*)selection);
  		return 1;
  	}
  	window_invalid(NULL);
  	break;
  case EVENT_NOTIFY_RESULT:
  	window_close();
  	break;
  default:
  	return 0;
  }

  return 1;
}