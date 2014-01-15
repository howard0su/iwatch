#include <stdint.h>
#include <msp430.h>
#pragma location="RAMCODE"
//------------------------------------------------------------------------------
// This portion of the code is first stored in Flash and copied to RAM then
// finally executes from RAM.
//-------------------------------------------------------------------------------
void write_block_int(void)
{
  unsigned int i;
  unsigned long * Flash_ptr;
  Flash_ptr = (unsigned long *)0x10000;     // Initialize write address
  __disable_interrupt();                    // 5xx Workaround: Disable global
                                            // interrupt while erasing. Re-Enable
                                            // GIE if needed

  // Erase Flash
  while(BUSY & FCTL3);                      // Check if Flash being used
  FCTL3 = FWKEY;                            // Clear Lock bit
  FCTL1 = FWKEY+ERASE;                      // Set Erase bit
  *(unsigned int *)Flash_ptr = 0;           // Dummy write to erase Flash seg
  while(BUSY & FCTL3);                      // Check if Erase is done

  // Write Flash
  FCTL1 = FWKEY+BLKWRT+WRT;                 // Enable block write

  for(i = 0; i < 32; i++)
  {
    *Flash_ptr++ = i;                 // Write long int to Flash

    while(!(WAIT & FCTL3));                 // Test wait until ready for next byte
  }

  FCTL1 = FWKEY;                            // Clear WRT, BLKWRT
  while(BUSY & FCTL3);                      // Check for write completion
  FCTL3 = FWKEY+LOCK;                       // Set LOCK
}

