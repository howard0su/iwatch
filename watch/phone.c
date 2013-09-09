/*
 * This file implment the notification for the incoming phone
 */

#include "contiki.h"
#include "window.h"
#include "hfp.h"
#include "bluetooth.h"
#include <string.h>
#include <stdio.h>

static char phonenumber[20] = "Unknown";
static uint8_t mode = 0; // mode - 0: phone 1: Siri mode

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
		if (mode)
			GrStringDrawCentered(pContext, "Talking", -1, 72, 60, 0);
		else	
			GrStringDrawCentered(pContext, "Calling", -1, 72, 60, 0);
		window_button(pContext, KEY_UP, "Vol Up");
		window_button(pContext, KEY_DOWN, "Vol Down");
		window_button(pContext, KEY_EXIT, "Hang up");
	    // volume
	    char buf[32];
	    sprintf(buf, "Volume: %d", codec_getvolume());
		GrContextFontSet(pContext, &g_sFontBaby16);
		GrStringDrawCentered(pContext, buf, -1, 72, 120, 0);
	}
	else if (hfp_getstatus(HFP_CIND_CALLSETUP) != HFP_CIND_CALLSETUP_NONE)
	{
		GrStringDrawCentered(pContext, "Incoming Call", -1, 72, 60, 0);
		window_button(pContext, KEY_EXIT, "Reject");
		window_button(pContext, KEY_ENTER, "Pickup");
		// TODO: not implemented
		//window_button(pContext, KEY_DOWN, "SMS Reply"); 
	}
	else
	{
		GrStringDrawCentered(pContext, "Finished", -1, 72, 60, 0);	
		window_close();

		return; // don't need paint others
	}

	if (!mode)
	{
	    // draw the phone number
		GrContextFontSet(pContext, &g_sFontNova28b);
	    GrStringDrawCentered(pContext, phonenumber, -1, 72, 80, 0);
	}
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
		mode = (uint8_t)rparam;
		break;
	case EVENT_RING:
		if (lparam == 0xffff)
		{
			strcpy(phonenumber, rparam);
			window_invalid(NULL);			
		}
		else if ((lparam >> 8) == HFP_CIND_CALL ||
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
			|| hfp_getstatus(HFP_CIND_CALLSETUP) != HFP_CIND_CALLSETUP_NONE
			|| mode)
		{
			hfp_accept_call(0);
		}
	default:
		return 0;
	}

	return 1;
}
