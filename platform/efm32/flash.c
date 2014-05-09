/**
 * \file
 * Functions for reading and writing flash ROM.
 * \author Adam Dunkels <adam@sics.se>
 */

/* Copyright (c) 2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * $Id: flash.c,v 1.3 2010/11/15 21:52:54 adamdunkels Exp $
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

#include "contiki.h"
#include "contiki-conf.h"
#include <stdlib.h>

#include "flash.h"
#include "dev/watchdog.h"

#define FLASH_TIMEOUT 30
#define FLASH_REQ_TIMEOUT 150

#define INFOMEM_LO (uint16_t*)0xFF000
#define INFOMEM_HI (uint16_t*)0x100000

static uint16_t sfrie;

/*---------------------------------------------------------------------------*/
void
flash_setup(void)
{
  	/* disable all interrupts to protect CPU
     	   during programming from system crash */
  	__disable_irq();

 	/* Enables the flash controller for writing. */
  	MSC_Init();
  
  	/* Stop watchdog. */
  	watchdog_stop();
}
/*---------------------------------------------------------------------------*/
void
flash_done(void)
{
  	/* Enable interrupts. */  
  	MSC_Deinit();
  	__enable_irq();  	
  	watchdog_start();
}
/*---------------------------------------------------------------------------*/
static void
unlock_infomem(void)
{
	MSC->LOCK = MSC_LOCK_LOCKKEY_UNLOCK;
}
/*---------------------------------------------------------------------------*/
static void
lock_infomem(void)
{
	MSC->LOCK = MSC_LOCK_LOCKKEY_LOCK;
}
/*---------------------------------------------------------------------------*/
void
flash_clear(uint16_t *ptr)
{
  	uint8_t r;
	
  	/* If ptr is in infomem, we need to unlock it first. */
  	if(ptr >= INFOMEM_LO && ptr <= INFOMEM_HI) 
  	{
    		unlock_infomem();
  	}
  	
  	while(MSC->STATUS & MSC_STATUS_LOCKED) 
  	{
    		r++;  /* Wait for BUSY = 0, not needed
	     		 unless run from RAM */
  	}
  	
  	MSC_ErasePage(ptr);
 	
  	if(ptr >= INFOMEM_LO && ptr <= INFOMEM_HI) 
  	{
    		lock_infomem();
  	}
}
/*---------------------------------------------------------------------------*/
void
flash_write(uint16_t *ptr, uint32_t word)
{
  	/* If ptr is in infomem, we need to unlock it first. */
  	if(ptr >= INFOMEM_LO && ptr <= INFOMEM_HI) 
  	{
    		unlock_infomem();
  	}
	
	MSC_WriteWord((uint32_t *)ptr,word,4);

  	if(ptr >= INFOMEM_LO && ptr <= INFOMEM_HI) 
  	{
    		lock_infomem();
  	}
}
/*---------------------------------------------------------------------------*/

void
flash_writepage(uint16_t *addr, const uint16_t *data, uint8_t size)
{
  	/* If ptr is in infomem, we need to unlock it first. */
  	if(addr >= INFOMEM_LO && addr <= INFOMEM_HI) 
  	{
    		unlock_infomem();
  	}

	MSC_WriteWord((uint32_t *)addr,(void const *) data,size/4);  		

  	if(addr >= INFOMEM_LO && addr <= INFOMEM_HI) 
  	{
    		lock_infomem();
  	}
}