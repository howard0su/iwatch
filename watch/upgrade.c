/****************************************************************
*  Description: Implementation for upgrade firmware
*    History:
*      Jun Su          2013/6/21        Created
*
* Copyright (c) Jun Su, 2013
*
* This unpublished material is proprietary to Jun Su.
* All rights reserved. The methods and
* techniques described herein are considered trade secrets
* and/or confidential. Reproduction or distribution, in whole
* or in part, is forbidden except by express written permission.
****************************************************************/

#include "contiki.h"
#include "window.h"
#include "grlib/grlib.h"
#include "Template_Driver.h"
#include "rtc.h"
#include <stdio.h>

#pragma segment="FLASHCODE"                 // Define flash segment code
#pragma segment="RAMCODE"

// Function prototypes
void copy_flash_to_RAM(void);
void write_block_int(void);

//------------------------------------------------------------------------------
// Copy flash function to RAM.
//------------------------------------------------------------------------------
void copy_flash_to_RAM(void)
{
  unsigned char *flash_start_ptr;           // Initialize pointers
  unsigned char *flash_end_ptr;
  unsigned char *RAM_start_ptr;

  //Initialize flash and ram start and end address
  flash_start_ptr = (unsigned char *)__segment_begin("FLASHCODE");
  flash_end_ptr = (unsigned char *)__segment_end("FLASHCODE");
  RAM_start_ptr = (unsigned char *)__segment_begin("RAMCODE");

  //calculate function size
  unsigned long function_size = (unsigned long)(flash_end_ptr) - (unsigned long)(flash_start_ptr);

  // Copy flash function to RAM
  memcpy(RAM_start_ptr,flash_start_ptr,function_size);
}

static uint8_t enter_bsl()
{
  copy_flash_to_RAM();                      // Copy flash to RAM function

  write_block_int();                        // This portion of code is executed

  while(1);
}

static enum {W4CONFIRM, CONFIRMED} state = W4CONFIRM;

static void onDraw(tContext *pContext)
{
	GrContextForegroundSet(pContext, ClrBlack);
	GrRectFill(pContext, &client_clip);
	GrContextForegroundSet(pContext, ClrWhite);

	GrContextFontSet(pContext, &g_sFontNova16);
	if (state == W4CONFIRM)
	  GrStringDrawWrap(pContext, "Plug watch to PC and press any key to continue", 0, 30, 150, 30);
	else
	{
	   GrStringDraw(pContext, "Run the flash program.", -1, 0, 30, 0);
	   GrStringDraw(pContext, "DO NOT UNPLUG", -1, 0, 90, 0);
	}
}

uint8_t upgrade_process(uint8_t ev, uint16_t lparam, void* rparam)
{
  // enable 
  switch(ev)
  {
  case EVENT_WINDOW_PAINT:
  	onDraw((tContext*)rparam);

  	if (state == CONFIRMED)
  		window_timer(CLOCK_SECOND);
  	break;
  case EVENT_KEY_PRESSED:
  	state = CONFIRMED;
  	window_invalid(NULL);
  	break;
  case PROCESS_EVENT_TIMER:
  	enter_bsl();
  	break;
  default:
  	return 0;
  }
  return 1;
}