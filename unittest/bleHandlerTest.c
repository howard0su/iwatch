
#include "CuTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "TestUtility/stlv_test_stub.h"

#include "ble_handler.h"

#define BLE_HANDLE_SEED_BEGIN 0x000b
#define BLE_HANDLE_SEED_END   0x0037

#define FILE_SIZE (8 * 1024 - 1)
#define FILE_NAME "/DATA/14-03-03"
static uint8_t s_test_buffer_in[20] = {0};
static uint8_t s_test_buffer_out[20] = {0};

static void TestBleFile(CuTest* tc)
{
    cfs_coffee_format();

    //write process
    printf("Write('W')\n");
    s_test_buffer_in[0] = 'W';
    sprintf((char*)&s_test_buffer_in[1], FILE_NAME);
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
    CuAssertIntEquals(tc, 'H', s_test_buffer_out[0]);

    uint16_t blockid = 0;
    while (1)
    {
        printf("Write('S')\n");
        //write desc
        s_test_buffer_in[0] = 'S';
        {
            //block id
            *((uint16_t*)&s_test_buffer_in[2]) = blockid;
            uint16_t pos = blockid * 20;
            if (FILE_SIZE < pos)
                break;

            //size
            uint16_t sizeleft = FILE_SIZE - pos;
            if (sizeleft > 20)
                sizeleft = 20;
            s_test_buffer_in[1] = sizeleft;

            //file name
            sprintf((char*)&s_test_buffer_in[4], FILE_NAME);
        }
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        CuAssertIntEquals(tc, 'P', s_test_buffer_out[0]);

        printf("WriteData(%d)\n", blockid);
        //write data
        for (int i = 0; i < sizeof(s_test_buffer_in); ++i)
            s_test_buffer_in[i] = (uint8_t)('A' + blockid);
        att_handler(BLE_HANDLE_FILE_DATA, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        CuAssertIntEquals(tc, 'O', s_test_buffer_out[0]);

        //next block
        blockid++;
    }
    //write command "Investigation"
    s_test_buffer_in[0] = 'C';
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

    //read process
    uint16_t read_block_id = 0;

    //write command "Investigation"
    s_test_buffer_in[0] = 'I';
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

    //read back desc
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
    CuAssertIntEquals(tc, 'F', s_test_buffer_out[0]);
    CuAssertStrEquals(tc, FILE_NAME, (char*)&s_test_buffer_out[4]);

    while (1)
    {
        //write command "Read"
        s_test_buffer_in[0] = 'R';
        s_test_buffer_in[1] = 0;
        *((uint16_t*)&s_test_buffer_in[2]) = read_block_id; 
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        if (s_test_buffer_out[0] == 'D')
        {
            CuAssertStrEquals(tc, FILE_NAME, (char*)&s_test_buffer_out[4]);
            CuAssertIntEquals(tc, read_block_id, *((uint16_t*)&s_test_buffer_out[2]));
            uint8_t size = s_test_buffer_out[1];

            if (FILE_SIZE > read_block_id * 20)
            {
                if (FILE_SIZE - read_block_id * 20 >= 20)
                {
                    CuAssertIntEquals(tc, 20, size);
                }
                else
                {
                    uint8_t lsize = FILE_SIZE - read_block_id * 20;
                    CuAssertIntEquals(tc, lsize, size);
                }
            }

            //read back desc
            att_handler(BLE_HANDLE_FILE_DATA, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ); 
            for (int i = 0; i < size; ++i)
            {
                CuAssertIntEquals(tc, 'A' + read_block_id, s_test_buffer_out[i]);
            }

            read_block_id++;
        }
        else if (s_test_buffer_out[0] == 'E')
        {
            return;
        }
        else
        {
            CuAssertTrue(tc, 0);
        }

    }
}

CuSuite* BleHandlerTestGetSuite(void)
{
    CuSuite* suite = CuSuiteNew("BLE Handler Test");
    SUITE_ADD_TEST(suite, TestBleFile); 
    return suite;
}

