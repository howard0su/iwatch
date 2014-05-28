#ifndef _SPIFLASH_H
#define _SPIFLASH_H
#include <stdint.h>

#define SPI_FLASH_PageSize			256                                  //?
#define SPI_FLASH_PerWritePageSize      	256                          //–?

/*W25X﹚?*/
#define W25X_WriteStatusReg		      	0x01                            //???盚竟
#define W25X_PageProgram		      	0x02                            //??
#define W25X_ReadData			      	0x03                            //??誹
#define W25X_WriteDisable		      	0x04                            //?ア
#define W25X_ReadStatusReg		      	0x05                            //???盚竟
#define W25X_WriteEnable		      	0x06                            //?ㄏ

#define W25X_FastReadData		      	0x0B                            //е硉?家Α
#define W25X_FastReadDual		      	0x3B                            //е???    
#define W25X_BlockErase64		      	0xD8                            //64KB?揽埃
#define W25X_BlockErase32		      	0x52                            //32KB?揽埃
#define W25X_SectorErase		      	0x20                            //?揽埃
#define W25X_ChipErase			      	0xC7                            //揽埃
#define W25X_PowerDown			      	0xB9                            //?奔?家Α
#define W25X_ReleasePowerDown	      		0xAB                            //癶奔?家Α
#define W25X_DeviceID			      	0xAB                            //?ID
#define W25X_ManufactDeviceID   	  	0x90                            //?硑ID
#define W25X_JedecDeviceID		      	0x9F 
#define W25X_EnableReset			0x66
#define W25X_Reset			  	0x99
#define W25X_ReadStatusReg2		      	0x35

#define WIP_Flag                      		0x01                            //?Γ?в
#define WEL_Flag				0x02                            //Write Enable
#define Dummy_Byte                    		0x00                            //?誹
//#define Dummy_Byte                    		0xFF                            //?誹

/******************************************************************************************
*ㄧ?SPI_FLASH_Init()
* ??void
* void
* SPIFLASH﹍てㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_Init(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_SectorErase()
* ??uint32_t SectorAddr   ?
* void
* SPIFLASH?揽埃ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_SectorErase(uint32_t SectorAddr, uint32_t size);
/******************************************************************************************
*ㄧ?SPI_FLASH_BulkErase()
* ??void
* void
* SPIFLASH俱揽埃ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_BulkErase(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_PageWrite()
* ??uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite ?誹?????
* void
* SPIFLASH???誹ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
/******************************************************************************************
*ㄧ?SPI_FLASH_BufferWrite()
* ??uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite ?誹?????
* void
* SPIFLASH??誹ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
/******************************************************************************************
*ㄧ?SPI_FLASH_BufferRead()
* ??uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead ?誹?????
* void
* SPIFLASH??誹ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void SPI_FLASH_BufferRead_Raw(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
/******************************************************************************************
*ㄧ?SPI_FLASH_ReadID()
* ??void
* uint32_t 竟ンID
* SPIFLASH?IDㄧ?场?ノ
*********************************************************************************************/
uint32_t SPI_FLASH_ReadID(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_ReadDeviceID()
* ??void
* uint32_t ??ID
* SPIFLASH???IDㄧ?场?ノ
*********************************************************************************************/
uint32_t SPI_FLASH_ReadDeviceID(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_StartReadSequence()
* ??uint32_t ReadAddr 24?
* void
* SPIFLASH??﹍ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
/******************************************************************************************
*ㄧ?SPI_FLASH_ReadByte()
* ??void
* uint8_t 8?誹
* SPIFLASH???ㄧ?场?ノ
*********************************************************************************************/
uint8_t SPI_FLASH_ReadByte(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_SendHalfWord()
* ??uint16_t HalfWord        ?16?誹
* uint16_t 16?誹
* SPIFLASH??16?誹ㄧ?场?ノ
*********************************************************************************************/
uint16_t SPI_FLASH_SendHalfWord(uint16_t HalfWord);
/******************************************************************************************
*ㄧ?SPI_FLASH_WriteEnable()
* ??void
* void
* SPIFLASH?ㄏㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_WriteEnable(void);
/******************************************************************************************
*ㄧ?SPI_FLASH_WaitForWriteEnd()
* ??void
* void
* SPIFLASH单?Ч?ㄧ?场?ノ
*********************************************************************************************/
void SPI_FLASH_WaitForWriteEnd(void);
/******************************************************************************************
*ㄧ?SPI_Flash_PowerDown()
* ??void
* void
* SPIFLASH?奔?家Αㄧ?场?ノ
*********************************************************************************************/
void SPI_Flash_PowerDown(void);
/******************************************************************************************
*ㄧ?SPI_Flash_WAKEUP()
* ??void
* void
* SPIFLASH?眶奔?家Αㄧ?场?ノ
*********************************************************************************************/
void SPI_Flash_WAKEUP(void);

/******************************************************************************************
*ㄧ?SPI_Flash_Reset()
* ??void
* void
* SPIFLASH Resetㄧ?场?ノ
*********************************************************************************************/
void SPI_Flash_Reset(void);
#endif