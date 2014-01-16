#include <stdint.h>
#include <msp430.h>
#include "spiflash.h"
#include "board.h"

#pragma segment="FLASHCODE"                 // Define flash segment code
#pragma segment="RAMCODE"


static inline uint8_t SPI_FLASH_SendByte( uint8_t data )
{
  UCA1TXBUF = data;
  while(( UCA1IFG & UCRXIFG ) == 0x00 ); // Wait for Rx completion (implies Tx is also complete)
  return( UCA1RXBUF );
}

static inline void SPI_FLASH_SendCommandAddress(uint8_t opcode, uint32_t address)
{
  SPI_FLASH_SendByte(opcode);
  SPI_FLASH_SendByte((address >> 16) & 0xFF);
  SPI_FLASH_SendByte((address >> 8) & 0xFF);
  SPI_FLASH_SendByte(address & 0xFF);
}

static inline void SPI_FLASH_CS_LOW()
{
  UCA1CTL1 &= ~UCSWRST;
  SPIOUT &= ~CSPIN;
}

static inline void SPI_FLASH_CS_HIGH()
{
  while(UCA1STAT & UCBUSY);
  UCA1CTL1 |= UCSWRST;
  SPIOUT |= CSPIN;
  __delay_cycles(10);
}

typedef enum 
{
    STATE_NEEDSIGNATURE,
    STATE_NEEDADDR,
    STATE_WRITE,
    STATE_DONE
}WriteState;

#define INTERRUPT_VECTOR_START 0xFFE0
#define SIGNATURE 0xFACE0001

//------------------------------------------------------------------------------
// This portion of the code is first stored in Flash and copied to RAM then
// finally executes from RAM.
//-------------------------------------------------------------------------------
#pragma location="RAMCODE"
void write_block_int()
{
  const uint32_t ReadAddr = 4UL * 1024 * 1024 - 256UL * 1024; // last 256KB position

  unsigned int i;
  char *pBuffer;
  WriteState state;

  unsigned int NumByteToRead;
  unsigned int NumByteToWrite;

  unsigned long * write_ptr;
  uint32_t buffer[32];
  
  write_ptr = (unsigned long *)INTERRUPT_VECTOR_START;     // Initialize write address

  __disable_interrupt();                    // 5xx Workaround: Disable global
                                            // interrupt while erasing. Re-Enable
                                            // GIE if needed

  state = STATE_NEEDSIGNATURE;
  // Start the loop
  while(state != STATE_DONE)
  {
    switch(state)
    {
      case STATE_NEEDSIGNATURE:
      NumByteToRead = 4;
      SPI_FLASH_CS_LOW();
      SPI_FLASH_SendCommandAddress(W25X_ReadData, ReadAddr);

      break;
      case STATE_NEEDADDR:
      NumByteToRead = 8; // one address and one 
      break;
      case STATE_WRITE:
      if (NumByteToWrite > 128)
        NumByteToRead = 128;
      else
        NumByteToRead = NumByteToWrite;
      break;
    }
 
    pBuffer = (char*)&buffer[0];;
    while (NumByteToRead--)
    {
      *pBuffer = SPI_FLASH_SendByte(Dummy_Byte);
      pBuffer++;
    }

    switch(state)
    {
      case STATE_NEEDSIGNATURE:
      {
        if (buffer[0] != SIGNATURE)
        {
          SPI_FLASH_CS_HIGH();
          return; // indicate there is error, just return
        }
        // Erase Flash
        while(BUSY & FCTL3);                      // Check if Flash being used
        FCTL3 = FWKEY;                            // Clear Lock bit
        FCTL1 = FWKEY+ERASE+MERAS;                // Set Erase bit
        *(unsigned int *)write_ptr = 0;           // Dummy write to erase All Flash seg
        while(BUSY & FCTL3);                      // Check if Erase is done

        state = STATE_NEEDADDR;
      }
      break;
      case STATE_NEEDADDR:
      {
        if (buffer[0] == 0)
        {
          state = STATE_DONE;
          continue;
        }

        // first uint32 is start address, second uint32 is length
        write_ptr = (unsigned long *)buffer[0];
        NumByteToWrite = buffer[1];
        state = STATE_WRITE;
      }
      break;
      case STATE_WRITE:
      {
        // Write Flash
        FCTL1 = FWKEY+BLKWRT+WRT;                 // Enable block write
        for(i = 0; i < NumByteToRead; i++)
        {
          *write_ptr++ = buffer[i];                 // Write long int to Flash

          while(!(WAIT & FCTL3));                 // Test wait until ready for next byte
        }

        FCTL1 = FWKEY;                            // Clear WRT, BLKWRT
        while(BUSY & FCTL3);                      // Check for write completion
        FCTL3 = FWKEY+LOCK;                       // Set LOCK

        NumByteToWrite -= NumByteToRead;

        if (NumByteToWrite == 0)
          state = STATE_NEEDADDR;
      }
      break;
    }
  }

  // reboot
  WDTCTL = 0;
}

