/*
 * This file implment the notification for the incoming phone
 */

#include "contiki.h"
#include "window.h"
#include "hfp.h"
#include <string.h>

static char phonenumber[20];

/*
 * The dialog shows the option to accept the call, reject call
 * or send SMS (if we can get MNS works)
 * After that, give the option to hang up
 * The dialog will be show as an notification when callsetup = 1
 */
static enum
{
	STATE_RING,
	STATE_PICKSMS,
	STATE_CALLING
}state;

static void onDraw(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrBlack);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrWhite);

    // draw the phone number
    GrStringDraw(pContext, phonenumber, -1, 80, 80, 0);
}

static void handleKey(uint8_t key)
{
	switch(state)
	{
		case STATE_RING:
		/* ring, down/enter-> pick, up -> SMS, exit -> reject */
		switch(key)
		{
			case KEY_DOWN:
			case KEY_ENTER:
				state = STATE_CALLING;
				// notify hfp that we are accepting the call
				hfp_accept_call(1);
				break;

			case KEY_EXIT:
				hfp_accept_call(0);
				window_close();
				return;
			case KEY_UP:
				state = STATE_PICKSMS;
				hfp_accept_call(0);
				break;
		}
		break;

		// exit, hang up the call
		case STATE_CALLING:
		{
			switch(key)
			{
				case KEY_EXIT:
				{
					// hang the call
					break;
				}
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
	state = STATE_RING;
	break;
	case EVENT_RING_NUM:
	// get the calling number from the user
	strcpy(phonenumber, rparam);
	window_invalid(NULL);
	break;
	case EVENT_RING:
	if (lparam == 0)
	{
		// this is ringing, ingore it
	}
	else
	{
		// +CIEV K, M
		uint8_t ind = (uint8_t)(lparam >> 8);
		uint8_t value = (uint8_t)lparam;

		if (ind == HFP_CIND_CALLSETUP)
		{

		}
		else if (ind == HFP_CIND_CALL)
		{
			if (value == 1)
			{
				// call is established
				state = STATE_CALLING;
			}
		}
	}
	break;
	case EVENT_WINDOW_PAINT:
	onDraw((tContext*)rparam);
	break;
	case EVENT_KEY_PRESSED:
	handleKey((uint8_t)lparam);
	break;
	case EVENT_EXIT_PRESSED:
	handleKey(KEY_EXIT);
	break;
	default:
		return 0;
	}

	return 1;
}
