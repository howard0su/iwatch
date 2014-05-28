#include "contiki.h"
#include <stdio.h>
#include "sys/rtimer.h"
#include "platform-conf.h"
#include "spiflash.h"
//#include "isr_compat.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#define hexdump(...)
#else
#define PRINTF(...)
#define hexdump(...)
#endif

#define SFLASH_DO		0
#define SFLASH_DI		1
#define SFLASH_CLK		2
#define SFLASH_SCS		3

USART_TypeDef *spi = USART1;

uint8_t* masterRxBuffer;
uint16_t masterRxBufferSize;
volatile uint32_t masterRxBufferIndex;
volatile uint8_t  Rx_RAW_Flag;

static inline uint8_t SPI_FLASH_SendByte( uint8_t data )
{
	USART_Tx(spi, data);
	return USART_Rx(spi);
}

/**************************************************************************//**
 * @brief Setting up RX interrupts from USART1 RX
 * @param receiveBuffer points to where received data is to be stored
 * @param bytesToReceive indicates the number of bytes to receive
 *****************************************************************************/
void SFLASH_setupRXInt(uint8_t* receiveBuffer, uint16_t bytesToReceive)
{ 	
  	/* Setting up pointer and indexes */
  	masterRxBuffer      = receiveBuffer;
  	masterRxBufferSize  = bytesToReceive;
  	masterRxBufferIndex = 0;

  	/* Flushing rx */
  	spi->CMD = USART_CMD_CLEARRX;

  	/* Enable interrupts */
  	NVIC_ClearPendingIRQ(USART1_RX_IRQn);
  	NVIC_EnableIRQ(USART1_RX_IRQn);
  	spi->IEN = USART_IEN_RXDATAV;
}

static void SPI_FLASH_SendCommandAddress(uint8_t opcode, uint32_t address)
{
	SPI_FLASH_SendByte(opcode);
	SPI_FLASH_SendByte((address >> 16) & 0xFF);
	SPI_FLASH_SendByte((address >> 8) & 0xFF);
	SPI_FLASH_SendByte(address & 0xFF);
	
}

static inline void SPI_FLASH_CS_LOW()
{
	GPIO_PinOutClear(gpioPortD, SFLASH_SCS);	//SET SFLASH_SCS be 0(low)
}

static inline void SPI_FLASH_CS_HIGH()
{	
	GPIO_PinOutSet(gpioPortD, SFLASH_SCS);
}

void SPI_FLASH_SectorErase(uint32_t SectorAddr, uint32_t size)
{
  	uint8_t opcode;
  	PRINTF("SECTOR Erase\n");
  	if (size == 4 * 1024UL)
  	{
    		opcode = W25X_SectorErase;
  	}
  	else if (size == 32 * 1024UL)
  	{
    		opcode = W25X_BlockErase32;
  	}
  	else if (size == 64 * 1024UL)
  	{
    		opcode = W25X_BlockErase64;
  	}
  	else
  	{
    		
    		return;
  	}
 	 
  	
  	SPI_FLASH_WriteEnable();
  	
  	SPI_FLASH_CS_LOW();
  	
  	SPI_FLASH_SendCommandAddress(opcode, SectorAddr);
  	
  	SPI_FLASH_CS_HIGH();
  	
  	SPI_FLASH_WaitForWriteEnd();
  	PRINTF("Sector erase done\n");
}

/******************************************************************************************
*函数名：SPI_FLASH_BulkErase()
* 参数：void
* 返回值：void
* 功能：SPIFLASH整片擦除函数，外部调用
*********************************************************************************************/
void SPI_FLASH_BulkErase(void)
{
  	SPI_FLASH_WriteEnable();   	
  	SPI_FLASH_CS_LOW();  	  	
  	PRINTF("Chip Erase\n");
  	SPI_FLASH_SendByte(W25X_ChipErase);  	
  	SPI_FLASH_CS_HIGH();  	
  	SPI_FLASH_WaitForWriteEnd();
  	PRINTF("chip erase done\n");
}

/******************************************************************************************
*函数名：SPI_FLASH_PageWrite()
* 参数：u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite 数据指针，写入地址，写入的个数
* 返回值：void
* 功能：SPIFLASH页写入数据函数，外部调用
*********************************************************************************************/
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{  	
#ifdef NOTYET	
  	hexdump(pBuffer, NumByteToWrite);
#endif
	PRINTF("WRITE PAGE\n");	   	
  	SPI_FLASH_WriteEnable();  	
  	SPI_FLASH_CS_LOW();  	
  	SPI_FLASH_SendCommandAddress(W25X_PageProgram, WriteAddr);
  	
  	if(NumByteToWrite > SPI_FLASH_PerWritePageSize)
  	{
     		NumByteToWrite = SPI_FLASH_PerWritePageSize;
  	}
  	
  	while (NumByteToWrite--)
  	{   	    		
    		SPI_FLASH_SendByte(~(*pBuffer));    	
    		pBuffer++;
  	}
  	
  	SPI_FLASH_CS_HIGH();
  	PRINTF("Write page done\n");
  	SPI_FLASH_WaitForWriteEnd();
}

/******************************************************************************************
*函数名：SPI_FLASH_BufferWrite()
* 参数：u8* pBuffer, u32 WriteAddr, u16 NumByteToWrite 数据指针，写入地址，写入的个数
* 返回值：void
* 功能：SPIFLASH多个数据函数，外部调用
*********************************************************************************************/
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite)
{
	PRINTF("BUF WRITE\n");
  	uint8_t NumOfPage = 0, NumOfSingle = 0, Addr = 0, count = 0, temp = 0;
  	Addr = WriteAddr % SPI_FLASH_PageSize;                           
  	count = SPI_FLASH_PageSize - Addr;
  	NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;                
  	NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;               
  	if (Addr == 0) 
  	{
    		if (NumOfPage == 0) 
    		{
      			SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);      
    		}
    		else 
    		{       			
      			while (NumOfPage--)
      			{ 
        			
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);        			
        			WriteAddr +=  SPI_FLASH_PageSize;        			
        			pBuffer += SPI_FLASH_PageSize;
      			}       			
      			SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
    		}
  	}
  	else 
  	{
    		if (NumOfPage == 0) 
    		{
      			if (NumOfSingle > count) 
      			{
        			temp = NumOfSingle - count;             			
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);        			
        			WriteAddr +=  count;        			
        			pBuffer += count;        			
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, temp);
      			}
      			else  
      			{        			
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumByteToWrite);
      			}
    		}
    		else 
    		{
      			NumByteToWrite -= count;         
      			NumOfPage =  NumByteToWrite / SPI_FLASH_PageSize;  
      			NumOfSingle = NumByteToWrite % SPI_FLASH_PageSize;       			
      			SPI_FLASH_PageWrite(pBuffer, WriteAddr, count);      			
      			WriteAddr +=  count;      			
      			pBuffer += count;
       			
      			while (NumOfPage--)
      			{        			
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, SPI_FLASH_PageSize);        			
        			WriteAddr +=  SPI_FLASH_PageSize;        			
        			pBuffer += SPI_FLASH_PageSize;
      			}
      			
      			if (NumOfSingle != 0)
      			{
        			SPI_FLASH_PageWrite(pBuffer, WriteAddr, NumOfSingle);
      			}
    		}
  	}
  	PRINTF("BUF Write done\n");
}

/******************************************************************************************
*函数名：SPI_FLASH_BufferRead()
* 参数：u8* pBuffer, u32 ReadAddr, u16 NumByteToRead 数据指针，读出的地址，读出的个数
* 返回值：void
* 功能：SPIFLASH多个数据函数，外部调用
*********************************************************************************************/
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{  	
#if 0
  	PRINTF("BUFFER Read\n");
#endif
  	uint16_t n = NumByteToRead;
  	Rx_RAW_Flag = 1;	   	
  	SPI_FLASH_CS_LOW();
  
  	SPI_FLASH_SendCommandAddress(W25X_ReadData, ReadAddr);  	
  	while (NumByteToRead--) 
  	{
    		*pBuffer = ~SPI_FLASH_SendByte(Dummy_Byte);    	
    		pBuffer++;
  	}
  	Rx_RAW_Flag = 0;
  	SPI_FLASH_CS_HIGH();
#if 0
	PRINTF("BUF Read done\n");
#endif
#ifdef NOTYET	
  	hexdump(pBuffer - n, n);
#endif
}

/******************************************************************************************
*函数名：SPI_FLASH_BufferRead()
* 参数：u8* pBuffer, u32 ReadAddr, u16 NumByteToRead 数据指针，读出的地址，读出的个数
* 返回值：void
* 功能：SPIFLASH多个数据函数，外部调用
*********************************************************************************************/
void SPI_FLASH_BufferRead_Raw(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead)
{
  	
  	PRINTF("BUF READ RAW\n");
  	uint16_t n = NumByteToRead;
   
  	SPI_FLASH_CS_LOW();
  
  	SPI_FLASH_SendCommandAddress(W25X_ReadData, ReadAddr);

  	while (NumByteToRead--) 
  	{    
    		*pBuffer = SPI_FLASH_SendByte(Dummy_Byte);    
    		pBuffer++;
  	}
  
  	SPI_FLASH_CS_HIGH();
	PRINTF("Buf read RAW done\n");
#ifdef NOTYET	
  	hexdump(pBuffer - n, n);
#endif
}

/******************************************************************************************
*函数名：SPI_FLASH_ReadID()
* 参数：void
* 返回值：u32 器件ID
* 功能：SPIFLASH读取ID函数，外部调用
*********************************************************************************************/
uint32_t SPI_FLASH_ReadID(void)
{
	uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;	
	uint8_t i;

  	SPI_FLASH_CS_LOW();
  	PRINTF("READ ID\n");


  	Temp0 = SPI_FLASH_SendByte(Dummy_Byte);  
  	Temp1 = SPI_FLASH_SendByte(Dummy_Byte);   
  	Temp2 = SPI_FLASH_SendByte(Dummy_Byte);

  	SPI_FLASH_CS_HIGH();
 	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
  	return Temp;
}
/******************************************************************************************
*函数名：SPI_FLASH_ReadDeviceID()
* 参数：void
* 返回值：u32 设备ID
* 功能：SPIFLASH读取设备ID函数，外部调用
*********************************************************************************************/
uint32_t SPI_FLASH_ReadDeviceID(void)
{
  	uint8_t Temp = 0;
   	PRINTF("READ DEV ID\n");
  	SPI_FLASH_CS_LOW();
  	  	
  	SPI_FLASH_SendCommandAddress(W25X_DeviceID, 0UL);
  	
  	Temp = SPI_FLASH_SendByte(Dummy_Byte);
  
  	SPI_FLASH_CS_HIGH();
  	PRINTF("Read dev id done\n");
  	return Temp;
}

static void SPI_FLASH_WaitForFlag(uint8_t flag)
{
  	uint16_t tick = 0;
  	uint8_t FLASH_Status = 0;
  	do
  	{     		
    		SPI_FLASH_CS_LOW();
    		SPI_FLASH_SendByte(W25X_ReadStatusReg);    	
    		FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);
    		tick++;    	
    		SPI_FLASH_CS_HIGH();
    		
  	}while ((FLASH_Status & flag) == flag); /* 检测是否空闲*/

  	//PRINTF("operation takes %d ticks\n", tick);
}

/******************************************************************************************
*函数名：SPI_FLASH_WriteEnable()
* 参数：void
* 返回值：void
* 功能：SPIFLASH写使能函数，外部调用
*********************************************************************************************/
void SPI_FLASH_WriteEnable(void)
{
	PRINTF("CMD Write EN\n");   
  	SPI_FLASH_CS_LOW();
  	//USART_Tx(spi, W25X_WriteEnable);
  	SPI_FLASH_SendByte(W25X_WriteEnable);
  
  	SPI_FLASH_CS_HIGH();
	PRINTF("CMD Write EN done\n");   
  	/*check WEL */
#ifdef UNUSED  	
  	SPI_FLASH_WaitForFlag(WEL_Flag);
#endif  	
}

/******************************************************************************************
*函数名：SPI_FLASH_WaitForWriteEnd()
* 参数：void
* 返回值：void
* 功能：SPIFLASH等待写完毕函数，外部调用
*********************************************************************************************/
void SPI_FLASH_WaitForWriteEnd(void)
{
  	SPI_FLASH_WaitForFlag(WIP_Flag);
}

/******************************************************************************************
*函数名：SPI_Flash_PowerDown()
* 参数：void
* 返回值：void
* 功能：SPIFLASH进入掉电模式函数，外部调用
*********************************************************************************************/
void SPI_Flash_PowerDown(void)   
{ 
  
  	SPI_FLASH_CS_LOW();
  	  	
  	SPI_FLASH_SendByte(W25X_PowerDown);
  
  	SPI_FLASH_CS_HIGH();
}   

/******************************************************************************************
*函数名：SPI_Flash_WAKEUP()
* 参数：void
* 返回值：void
* 功能：SPIFLASH唤醒掉电模式函数，外部调用
*********************************************************************************************/
void SPI_Flash_WAKEUP(void)   
{
  
  	SPI_FLASH_CS_LOW();
  	  	
  	SPI_FLASH_SendByte(W25X_ReleasePowerDown);
  
  	SPI_FLASH_CS_HIGH();              
}  

/******************************************************************************************
*函数名：SPI_Flash_Reset()
* 参数：void
* 返回值：void
* 功能：SPIFLASH Reset函数，外部调用
*********************************************************************************************/
void SPI_Flash_Reset(void)   
{
  
  	SPI_FLASH_CS_LOW();  	
  	
  	SPI_FLASH_SendByte(W25X_EnableReset);
  
  	SPI_FLASH_CS_HIGH();
  
  	SPI_FLASH_CS_LOW();
  	
  	SPI_FLASH_SendByte(W25X_Reset);
  
  	SPI_FLASH_CS_HIGH();
  
}  

void USART1_setup(void)
{
  	USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;

  	init.baudrate     = 1000000;
  	init.refFreq	  = 48000000;
  	init.databits     = usartDatabits8;
  	init.msbf         = 1;
  	init.master       = 1;
  	init.clockMode    = usartClockMode3;
  	init.prsRxEnable  = 0;
  	init.autoTx       = 0;
	init.enable 	  = usartEnable;
  	USART_InitSync(USART1, &init);
}

void SPI_FLASH_Init(void)
{
	printf("Init SPIflash\n");
	/* SPI flash port/pin configuration */
	/* Pin PD0 is configure to Push-pull as SPI_DO. To avoid false start, configure output as high  */	
  	GPIO_PinModeSet(gpioPortD, SFLASH_DO, gpioModePushPull, 1);  	
  	/* Pin PD1 is configured to Input enabled as SPI_DI */
  	GPIO_PinModeSet(gpioPortD, SFLASH_DI, gpioModeInput, 0);  	
  	/* Pin PD2 is configured to Input enabled as SPI_SCLK */
  	GPIO_PinModeSet(gpioPortD, SFLASH_CLK, gpioModePushPull, 0);  	
  	/* Pin PD3 is configure to Push-pull as SPI_SCS. To avoid false start, configure output as high  */	
  	GPIO_PinModeSet(gpioPortD, SFLASH_SCS, gpioModePushPull, 1);  
  	
	/* Enable clock for USART1 */
  	CMU_ClockEnable(cmuClock_USART1, true);
  	/* Custom initialization for USART1 */
  	USART1_setup(); 
  	
  	/* Clearing old transfers/receptions, and disabling interrupts */
  	USART1->CMD = USART_CMD_CLEARRX | USART_CMD_CLEARTX;
  	USART1->IEN = 0;
  	//USART1->CTRL |= USART_CTRL_AUTOCS;	//
  	/* Module USART1 is configured to location 1 */
  	USART1->ROUTE = (USART1->ROUTE & ~_USART_ROUTE_LOCATION_MASK) | USART_ROUTE_LOCATION_LOC1;
  	/* Enable signals TX, RX, CLK, CS */
  	USART1->ROUTE |= USART_ROUTE_TXPEN | USART_ROUTE_RXPEN | USART_ROUTE_CLKPEN ;
  	
    	
    	/* Enabling Master, TX and RX */
    	USART1->CMD   = USART_CMD_MASTEREN | USART_CMD_TXEN | USART_CMD_RXEN;	
    	
    	/* Clear previous interrupts */
  	USART1->IFC = _USART_IFC_MASK;
	Rx_RAW_Flag = 0;
  	//SPI_Flash_Reset();

  	PRINTF("\n$SPIFLASH OK$\n");

	uint8_t FLASH_Status;
	uint8_t FLASH_ID;
  	SPI_FLASH_CS_LOW();
  	
  	
  	SPI_FLASH_SendByte(W25X_ReadStatusReg);
 	FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);
  
  	SPI_FLASH_CS_HIGH();
  	PRINTF("status register 1 = %x \n", FLASH_Status);

  	SPI_FLASH_CS_LOW();
  	
  	FLASH_Status = SPI_FLASH_SendByte(Dummy_Byte);
  
  	SPI_FLASH_CS_HIGH();
  	PRINTF("status register 2 = %x \n", FLASH_Status);  

	FLASH_ID = (uint8_t)SPI_FLASH_ReadDeviceID();
  	PRINTF("Find SPI Flash DeviceId = %2x \n", FLASH_ID);


}