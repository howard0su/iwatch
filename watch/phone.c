/*
 * This file implment the notification for the incoming phone
 */

#include "contiki.h"
#include "window.h"
#include "hfp.h"
#include "bluetooth.h"
#include <string.h>

static char phonenumber[20];

/*
 * The dialog shows the option to accept the call, reject call
 * or send SMS (if we can get MNS works)
 * After that, give the option to hang up
 * The dialog will be show as an notification when callsetup = 1
 */
static void onDraw(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrBlack);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrWhite);

	GrContextFontSet(pContext, &g_sFontBaby16);
	if (hfp_getstatus(HFP_CIND_CALL) == HFP_CIND_CALL_ACTIVE)
	{
		GrStringDraw(pContext, "Calling", 4, 40, 50, 0);
	}
	else if (hfp_getstatus(HFP_CIND_CALLSETUP) != HFP_CIND_CALLSETUP_NONE)
	{
		GrStringDraw(pContext, "Ring", 4, 40, 50, 0);
	}
	else
	{
		GrStringDraw(pContext, "Done", 4, 40, 50, 0);	
	}

	GrContextFontSet(pContext, &g_sFontBaby12);
    // draw the phone number
    GrStringDraw(pContext, phonenumber, -1, 20, 80, 0);

    // volume
    char buf[32];
    sprintf(buf, "Volume: %d", codec_getvolume());
	GrStringDrawCentered(pContext, buf, -1, 72, 120, 0);
}

static void handleKey(uint8_t key)
{
	switch(hfp_getstatus(HFP_CIND_CALL))
	{
		case HFP_CIND_CALL_NONE:
		/* ring, down/enter-> pick, up -> SMS, exit -> reject */
		switch(key)
		{
			case KEY_DOWN:
			case KEY_ENTER:
				// notify hfp that we are accepting the call
				hfp_accept_call(1);
				break;
			case KEY_UP:
				//hfp_accept_call(0);
				break;
		}
		break;

		// exit, hang up the call
		case HFP_CIND_CALL_ACTIVE:
		{
			switch(key)
			{
				case KEY_UP:
				{
					codec_changevolume(+1);
					break;
				}
				case KEY_DOWN:
				{
					// decrease voice
					codec_changevolume(-1);
					break;
				}
			}
		}

	}

	window_invalid(NULL);
}

uint8_t phone_process(uint8_t ev, uint16_t lparam, void* rparam)
{
	switch(ev)
	{
	case EVENT_WINDOW_CREATED:
		break;

	case EVENT_RING_NUM:
		// get the calling number from the user
		strcpy(phonenumber, rparam);
		window_invalid(NULL);
		break;

	case EVENT_RING:
		if ((lparam >> 8) == HFP_CIND_CALL ||
			(lparam >> 8) == HFP_CIND_CALLSETUP)
		{
			window_invalid(NULL);
		}
		break;
	case EVENT_WINDOW_PAINT:
		onDraw((tContext*)rparam);
		break;

	case EVENT_KEY_PRESSED:
		handleKey((uint8_t)lparam);
		break;

	case EVENT_WINDOW_CLOSING:
		if (hfp_getstatus(HFP_CIND_CALL) == HFP_CIND_CALL_ACTIVE
			|| hfp_getstatus(HFP_CIND_CALLSETUP) != HFP_CIND_CALLSETUP_NONE)
		{
			hfp_accept_call(0);
		}
	default:
		return 0;
	}

	return 1;
}
