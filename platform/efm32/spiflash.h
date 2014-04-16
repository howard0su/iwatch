#ifndef _SPIFLASH_H
#define _SPIFLASH_H
#include <stdint.h>

#define SPI_FLASH_PageSize			256                                  //?�j�p
#define SPI_FLASH_PerWritePageSize      	256                          //�C?�j�p

/*W25X���O�w?*/
#define W25X_WriteStatusReg		      	0x01                            //???�H�s��
#define W25X_PageProgram		      	0x02                            //??�J���O
#define W25X_ReadData			      	0x03                            //??�u���O
#define W25X_WriteDisable		      	0x04                            //?������O
#define W25X_ReadStatusReg		      	0x05                            //???�H�s��
#define W25X_WriteEnable		      	0x06                            //?�ϯ���O

#define W25X_FastReadData		      	0x0B                            //�ֳt?�Ҧ����O
#define W25X_FastReadDual		      	0x3B                            //��???�X���O    
#define W25X_BlockErase64		      	0xD8                            //64KB?�������O
#define W25X_BlockErase32		      	0x52                            //32KB?�������O
#define W25X_SectorErase		      	0x20                            //��?�������O
#define W25X_ChipErase			      	0xC7                            //���������O
#define W25X_PowerDown			      	0xB9                            //?�J��?�Ҧ����O
#define W25X_ReleasePowerDown	      		0xAB                            //�h�X��?�Ҧ�
#define W25X_DeviceID			      	0xAB                            //?�����ID
#define W25X_ManufactDeviceID   	  	0x90                            //?����yID
#define W25X_JedecDeviceID		      	0x9F 
#define W25X_EnableReset			0x66
#define W25X_Reset			  	0x99
#define W25X_ReadStatusReg2		      	0x35

#define WIP_Flag                      		0x01                            //?�J��?�Ӧ�
#define WEL_Flag				0x02                            //Write Enable
#define Dummy_Byte                    		0x00                            //��?�u
//#define Dummy_Byte                    		0xFF                            //��?�u

/******************************************************************************************
*��?�W�GSPI_FLASH_Init()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH��l�ƨ�?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_Init(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_SectorErase()
* ??�Guint32_t SectorAddr   ?�a�}
* ��^�ȡGvoid
* �\��GSPIFLASH��?������?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_SectorErase(uint32_t SectorAddr, uint32_t size);
/******************************************************************************************
*��?�W�GSPI_FLASH_BulkErase()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH���������?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_BulkErase(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_PageWrite()
* ??�Guint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite ?�u��?�A?�J�a�}�A?�J��??
* ��^�ȡGvoid
* �\��GSPIFLASH??�J?�u��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
/******************************************************************************************
*��?�W�GSPI_FLASH_BufferWrite()
* ??�Guint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite ?�u��?�A?�J�a�}�A?�J��??
* ��^�ȡGvoid
* �\��GSPIFLASH�h??�u��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
/******************************************************************************************
*��?�W�GSPI_FLASH_BufferRead()
* ??�Guint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead ?�u��?�A?�X���a�}�A?�X��??
* ��^�ȡGvoid
* �\��GSPIFLASH�h??�u��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
void SPI_FLASH_BufferRead_Raw(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
/******************************************************************************************
*��?�W�GSPI_FLASH_ReadID()
* ??�Gvoid
* ��^�ȡGuint32_t ����ID
* �\��GSPIFLASH?��ID��?�A�~��?��
*********************************************************************************************/
uint32_t SPI_FLASH_ReadID(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_ReadDeviceID()
* ??�Gvoid
* ��^�ȡGuint32_t ??ID
* �\��GSPIFLASH?��??ID��?�A�~��?��
*********************************************************************************************/
uint32_t SPI_FLASH_ReadDeviceID(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_StartReadSequence()
* ??�Guint32_t ReadAddr 24��?�a�}
* ��^�ȡGvoid
* �\��GSPIFLASH??�l��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
/******************************************************************************************
*��?�W�GSPI_FLASH_ReadByte()
* ??�Gvoid
* ��^�ȡGuint8_t 8��?�u
* �\��GSPIFLASH?�@?�r?��?�A�~��?��
*********************************************************************************************/
uint8_t SPI_FLASH_ReadByte(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_SendHalfWord()
* ??�Guint16_t HalfWord        ?�J��16��?�u
* ��^�ȡGuint16_t 16��?�u
* �\��GSPIFLASH??16��?�u��?�A�~��?��
*********************************************************************************************/
uint16_t SPI_FLASH_SendHalfWord(uint16_t HalfWord);
/******************************************************************************************
*��?�W�GSPI_FLASH_WriteEnable()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH?�ϯ��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_WriteEnable(void);
/******************************************************************************************
*��?�W�GSPI_FLASH_WaitForWriteEnd()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH����?��?��?�A�~��?��
*********************************************************************************************/
void SPI_FLASH_WaitForWriteEnd(void);
/******************************************************************************************
*��?�W�GSPI_Flash_PowerDown()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH?�J��?�Ҧ���?�A�~��?��
*********************************************************************************************/
void SPI_Flash_PowerDown(void);
/******************************************************************************************
*��?�W�GSPI_Flash_WAKEUP()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH?����?�Ҧ���?�A�~��?��
*********************************************************************************************/
void SPI_Flash_WAKEUP(void);

/******************************************************************************************
*��?�W�GSPI_Flash_Reset()
* ??�Gvoid
* ��^�ȡGvoid
* �\��GSPIFLASH Reset��?�A�~��?��
*********************************************************************************************/
void SPI_Flash_Reset(void);
#endif