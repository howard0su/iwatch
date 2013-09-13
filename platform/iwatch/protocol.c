#include "contiki.h"
#include <stdio.h>
#include "uart1.h"
#include <cfs/cfs.h>

PROCESS(protocol_process, "Protocol Handle");

/********************** Protocol Definition *************************/
// ret code
#define RX_PACKET_ONGOING 0x00
#define DATA_RECEIVED 0x01
#define RX_ERROR_RECOVERABLE 0x02
#define RX_ERROR_REINIT 0x03
#define RX_ERROR_FATAL 0x04
#define PI_DATA_RECEIVED 0x05

#define PI_COMMAND_UPPER 0x50

//errors
#define HEADER_INCORRECT (PI_COMMAND_UPPER + 0x01)
#define CHECKSUM_INCORRECT (PI_COMMAND_UPPER + 0x02)
#define PACKET_SIZE_ZERO (PI_COMMAND_UPPER + 0x03)
#define PACKET_SIZE_TOO_BIG (PI_COMMAND_UPPER + 0x04)
#define UNKNOWN_ERROR (PI_COMMAND_UPPER + 0x05)
// errors for PI commands
#define UNKNOWN_BAUD_RATE (PI_COMMAND_UPPER + 0x06)

// TA PI Commands
#define CHANGE_BAUD_RATE (PI_COMMAND_UPPER + 0x02)

#define MAX_BUFFER_SIZE 260

//Received commands
#define RX_DATA_BLOCK      0x10
#define RX_PASSWORD        0x11
#define ERASE_SEGMENT      0x12
#define TOGGLE_INFO        0x13
#define ERASE_BLOCK        0x14
#define MASS_ERASE         0x15
#define CRC_CHECK          0x16
#define LOAD_PC            0x17
#define TX_DATA_BLOCK      0x18
#define TX_BSL_VERSION     0x19
#define TX_BUFFER_SIZE     0x1A
#define RX_DATA_BLOCK_FAST 0x1B

#define TX_FILE_BEGIN      0x30
#define TX_FILE_END        0x31
#define TX_FILE_BLOCK      0x32
#define TX_FILE_REMOVE     0x33

#define TX_LOG_GET         0x34

//Responses
#define BSL_DATA_REPLY    0x3A
#define BSL_MESSAGE_REPLY 0x3B
#define ACK               SUCCESSFUL_OPERATION

// Error Codes
// - From the API
#define SUCCESSFUL_OPERATION 0x00
#define NO_EXCEPTIONS 0x00
#define MEMORY_WRITE_CHECK_FAILED 0x01
#define FLASH_FAIL_BIT_SET 0x02
#define VOLTAGE_CHANGE_DURING_PROGRAM 0x03
#define BSL_LOCKED 0x04
#define BSL_PASSWORD_ERROR 0x05
#define BYTE_WRITE_FORBIDDEN 0x06
// - From the Command Interpreter
#define UNKNOWN_COMMAND 0x07
#define LENGTH_TOO_BIG_FOR_BUFFER 0x08

#define sendByte(data) uart_sendByte(data)
#define getByte() uart_getByte()

extern void PI_sendData(int size);

char *BSL430_ReceiveBuffer;
char *BSL430_SendBuffer;
unsigned int BSL430_BufferSize;

char RAM_Buf[MAX_BUFFER_SIZE];
char Log_Buf[160];
static uint8_t LogRead, LogWrite;

void protocol_init()
{
    BSL430_ReceiveBuffer = RAM_Buf;
    BSL430_SendBuffer = RAM_Buf;

    LogRead = 0;
    LogWrite = 1;
}

void protocol_start(uint8_t start)
{
	if (start)
		process_start(&protocol_process, NULL);
	else
		process_exit(&protocol_process);
}

int putchar(int data)
{
    int x = splhigh();
    Log_Buf[LogWrite++] = data;
    if (LogWrite >= sizeof(Log_Buf)) LogWrite = 0;
    //if (LogRead == LogWrite) LogRead = LogWrite;
    splx(x);
    return data;
}

char verifyData(int checksum);
void interpretCommand();
void interpretPI_Command();

PROCESS_THREAD(protocol_process, ev, data)
{
  static char RX_StatusFlags;
  static char dataByte;
  static int dataPointer;
  static volatile int checksum;

  PROCESS_BEGIN();

  while(1)
  {
    RX_StatusFlags = RX_PACKET_ONGOING;
    dataByte = 0;
    dataPointer = 0;
    checksum = 0;

    while (RX_StatusFlags == RX_PACKET_ONGOING)
    {
        PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);
        dataByte = getByte();                            // get another byte from host
        if (dataPointer == 0)                                // first byte is the size of the Core
                                                             // packet
        {
            if (dataByte != 0x80)                            // first byte in packet should be 0x80
            {
                sendByte(HEADER_INCORRECT);
                RX_StatusFlags = RX_ERROR_RECOVERABLE;
            }
            else
            {
                dataPointer++;
            }
        }
        else if (dataPointer == 1)                           // first byte is the size of the Core
                                                             // packet
        {
            BSL430_BufferSize = dataByte;
            dataPointer++;
        }
        else if (dataPointer == 2)
        {
            BSL430_BufferSize |= (int)dataByte << 8;
            if (BSL430_BufferSize == 0)
            {
                sendByte(PACKET_SIZE_ZERO);
                RX_StatusFlags = RX_ERROR_RECOVERABLE;
            }
            if (BSL430_BufferSize > MAX_BUFFER_SIZE)         // For future devices that might need
                                                             // smaller packets
            {
                sendByte(PACKET_SIZE_TOO_BIG);
                RX_StatusFlags = RX_ERROR_RECOVERABLE;
            }
            dataPointer++;
        }
        else if (dataPointer == (BSL430_BufferSize + 3))
        {
            // if the pointer is pointing to the Checksum low data byte which resides
            // after 0x80, rSize, Core Command.
            checksum = dataByte;
            dataPointer++;
        }
        else if (dataPointer == (BSL430_BufferSize + 4))
        {
            // if the pointer is pointing to the Checksum low data byte which resides
            // after 0x80, rSize, Core Command, CKL.
            checksum = checksum | dataByte << 8;
            if (verifyData(checksum))
            {
                if ((RAM_Buf[0] & 0xF0) == PI_COMMAND_UPPER)
                {
                    interpretPI_Command();
                    RX_StatusFlags = RX_PACKET_ONGOING;
                    dataByte = 0;
                    dataPointer = 0;
                    checksum = 0;
                }
                else
                {
                    sendByte(ACK);
                    RX_StatusFlags = DATA_RECEIVED;
                }
            }
            else
            {
                sendByte(CHECKSUM_INCORRECT);
                RX_StatusFlags = RX_ERROR_RECOVERABLE;
            }
        }
        else
        {
            RAM_Buf[dataPointer - 3] = dataByte;
            dataPointer++;
        }
    }

    if (RX_StatusFlags & DATA_RECEIVED)
    {
      interpretCommand();
    }
  }

  PROCESS_END();
}


/*******************************************************************************
* *Function:    sendMessage
* *Description: Sends a Reply message with attached information
* *Parameters:
*           char message    the message to send
*******************************************************************************/

void sendMessage(char message)
{
    BSL430_SendBuffer[0] = BSL_MESSAGE_REPLY;
    BSL430_SendBuffer[1] = message;
    PI_sendData(2);
}

/*******************************************************************************
* *Function:    sendData
* *Description: Sends the data in the data buffer
* *Parameters:
*           int size    the number of bytes in the buffer
*******************************************************************************/

void PI_sendData(int size)
{
    int i;

    sendByte(0x80);
    sendByte(size & 0xFF);
    sendByte(size >> 8 & 0xFF);
    CRCINIRES = 0xFFFF;
    for (i = 0; i < size; i++){
        CRCDIRB_L = RAM_Buf[i];
        sendByte(RAM_Buf[i]);
    }
    i = CRCINIRES;
    sendByte(i & 0xFF);
    sendByte(i >> 8 & 0xFF);
}


/*******************************************************************************
* *Function:    verifyData
* *Description: verifies the data in the data buffer against a checksum
* *Parameters:
*           int checksum    the checksum to check against
****************Returns:
*           1 checksum parameter is correct for data in the data buffer
*           0 checksum parameter is not correct for the data in the buffer
*******************************************************************************/

char verifyData(int checksum)
{
    int i;

    CRCINIRES = 0xFFFF;
    for (i = 0; i < BSL430_BufferSize; i++)
    {
        CRCDIRB_L = RAM_Buf[i];
    }
    return (CRCINIRES == checksum);
}


int PI_getBufferSize()
{
    return MAX_BUFFER_SIZE;
}

/*******************************************************************************
* *Function:    sendDataBlock
* *Description: Fills the SendBuffer array with bytes from the given parameters
*             Sends the data by calling the PI, or sends an error
****************Parameters:
*           unsigned long addr    The address from which to begin reading the block
*           int length            The number of bytes to read
*******************************************************************************/

void sendDataBlock(const char* addr, unsigned int length)
{
    const char* endAddr = addr + length;
    unsigned int bytes;

    while (addr < endAddr)
    {
        if ((endAddr - addr) > PI_getBufferSize() - 1)
        {
            bytes = PI_getBufferSize() - 1;
        }
        else
        {
            bytes = (endAddr - addr);
        }

        for(int i = 0; i < bytes; i++)
            BSL430_SendBuffer[i + 1] = addr[i];
        BSL430_SendBuffer[0] = BSL_DATA_REPLY;
        PI_sendData(bytes + 1);
        addr += bytes;
    }
}

const unsigned char PROTOCOL_Version[4] = { 0x01, 0x00, 0x00, 0x02 };

int fd_handle;

void file_begin(char *name, uint8_t length)
{
    name[length] = 0;

    fd_handle = cfs_open(name, CFS_WRITE);
    if (fd_handle == -1)
        sendMessage(UNKNOWN_ERROR);
    else
        sendMessage(ACK);
}

void interpretCommand()
{
    unsigned char command = BSL430_ReceiveBuffer[0];
    unsigned long addr = BSL430_ReceiveBuffer[1];

    addr |= ((unsigned long)BSL430_ReceiveBuffer[2]) << 8;
    addr |= ((unsigned long)BSL430_ReceiveBuffer[3]) << 16;
    /*----------------------------------------------------------------------------*/
    switch (command)
    {
        case TX_BSL_VERSION:              // Transmit BSL Version array
            sendDataBlock(PROTOCOL_Version, 4);
            break;
        case TX_FILE_BEGIN:
            //file_begin(BSL430_ReceiveBuffer[4])
            break;
        case TX_LOG_GET:
            sendDataBlock(Log_Buf, LogWrite);
            LogWrite = 0;
            break;
        default:
            sendMessage(UNKNOWN_COMMAND);
    }
}

void interpretPI_Command()
{
    char command = RAM_Buf[0];

    if (command == CHANGE_BAUD_RATE)
    {
        char rate = RAM_Buf[1];
        if (rate > BAUD_4800 && rate <= BAUD_115200)
        {
            sendMessage(ACK);
            uart_changerate(rate);
        }
        else
            sendMessage(UNKNOWN_BAUD_RATE);
    }
}