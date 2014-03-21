
#include "CuTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "TestUtility/stlv_test_stub.h"

#include "ble_handler.h"
#include "watch/sportsdata.h"

#define BLE_HANDLE_SEED_BEGIN 0x000b
#define BLE_HANDLE_SEED_END   0x0037

#define FILE_SIZE (8 * 1024 - 1)
#define FILE_NAME "/DATA/14-03-03"
static uint8_t s_test_buffer_in[20] = {0};
static uint8_t s_test_buffer_out[20] = {0};

static void dumpBuffer(uint8_t* buf, int size)
{
    for (int i = 0; i < size; ++i)
    {
        if (i > 0 && i % 16 == 0)
            printf("\n");
        printf("%02x ", buf[i]);
    }
    if (size > 0)
        printf("\n");
}

int TestReadBleFile(CuTest* tc, char* filename, int unitsize)
{
    //write command "reset"
    printf("write FILE_DESC:X\n"); 
    s_test_buffer_in[0] = 'X';
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);
        //read process
    uint16_t read_block_id = 0;

    //write command "Investigation"
    printf("write FILE_DESC:I\n", 0, read_block_id); 
    s_test_buffer_in[0] = 'I';
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

    //read back desc
    printf("read FILE_DESC:\n");
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
    dumpBuffer(s_test_buffer_out, sizeof(s_test_buffer_out));
    CuAssertIntEquals(tc, 'F', s_test_buffer_out[0]);
    CuAssertStrEquals(tc, filename, (char*)&s_test_buffer_out[4]);

    while (1)
    {
        //write command "Read"
        printf("write FILE_DESC:R, %d, %d\n", 0, read_block_id);
        memcpy(s_test_buffer_in, s_test_buffer_out, 20);
        s_test_buffer_in[0] = 'R';
        s_test_buffer_in[1] = read_block_id;
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        printf("read FILE_DESC:\n");
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        dumpBuffer(s_test_buffer_out, sizeof(s_test_buffer_out));
        if (s_test_buffer_out[0] == 'D')
        {
            //CuAssertStrEquals(tc, filename, (char*)&s_test_buffer_out[4]);
            CuAssertIntEquals(tc, read_block_id, s_test_buffer_out[1]);
            uint8_t size = s_test_buffer_out[1];

            /*
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
            */

            //read back data
            att_handler(BLE_HANDLE_FILE_DATA, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
            printf("read FILE_DATA:\n");
            dumpBuffer(s_test_buffer_out, sizeof(s_test_buffer_out));

            read_block_id++;
        }
        else if (s_test_buffer_out[0] == 'E')
        {
            return read_block_id;
        }
        else
        {
            CuAssertTrue(tc, 0);
            return -1;
        }

    }

    return -1;
}

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
    while (blockid < 20)
    {
        printf("Write('S')\n");
        //write desc
        s_test_buffer_in[0] = 'S';
        {
            //block id
            s_test_buffer_in[1] = blockid;
            uint16_t pos = blockid * 20;
            if (FILE_SIZE < pos)
                break;

            //size
            uint16_t sizeleft = FILE_SIZE - pos;
            if (sizeleft > 19)
                sizeleft = 19;
            *((uint16_t*)&s_test_buffer_in[2]) = sizeleft;

            //file name
            sprintf((char*)&s_test_buffer_in[4], FILE_NAME);
        }
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        CuAssertIntEquals(tc, 'P', s_test_buffer_out[0]);

        printf("WriteData(%d)\n", blockid);
        //write data
        for (int i = 1; i < sizeof(s_test_buffer_in); ++i)
            s_test_buffer_in[i] = (uint8_t)('A' + blockid);
        s_test_buffer_in[0] = 0;
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

    TestReadBleFile(tc, FILE_NAME, 0);
}

static void TestBleSportsData(CuTest* tc)
{
    cfs_coffee_format();
    init_send_pack_stub(); 

    uint8_t meta[] = {DATA_COL_STEP, DATA_COL_DIST, DATA_COL_HR};
    uint32_t data[] = {1234, 5678, 9012};
    create_data_file(12, 7, 9);
    write_data_line(0x00, 1, 1, meta, data, 3);
    write_data_line(0x00, 1, 2, meta, data, 3);
    write_data_line(0x01, 1, 3, meta, data, 3);
    write_data_line(0x02, 1, 4, meta, data, 3);
    write_data_line(0x03, 1, 5, meta, data, 3);
    close_data_file();

    int ret = TestReadBleFile(tc, "/DATA/12-07-09", 0);
    CuAssertIntEquals(tc, 6, ret);
}

static void TestBleFile2(CuTest* tc)
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
    while (blockid < 10)
    {
        printf("Write('S')\n");
        uint16_t sizeleft = 0;
        //write desc
        s_test_buffer_in[0] = 'S';
        {
            //block id
            s_test_buffer_in[1] = blockid;
            uint16_t pos = blockid * 20;
            if (FILE_SIZE < pos)
                break;

            //size
            sizeleft = FILE_SIZE - pos;
            if (sizeleft > 1200)
                sizeleft = 1200;
            *((uint16_t*)&s_test_buffer_in[2]) = sizeleft;

            //file name
            sprintf((char*)&s_test_buffer_in[4], FILE_NAME);
        }
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        CuAssertIntEquals(tc, 'P', s_test_buffer_out[0]);

        printf("WriteData(%d)\n", blockid);
        //write data
        for (int i = 1; i < sizeof(s_test_buffer_in); ++i)
            s_test_buffer_in[i] = (uint8_t)('A' + blockid);

        int subblockid = 0;
        for (int i = 0; i < sizeleft; i += 19)
        {
            s_test_buffer_in[0] = subblockid;
            att_handler(BLE_HANDLE_FILE_DATA, 0, 
                s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);
            subblockid++;
        }

        //read back desc
        att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_out, sizeof(s_test_buffer_out), ATT_HANDLE_MODE_READ);
        CuAssertIntEquals(tc, 'O', s_test_buffer_out[0]);

        //next block
        blockid++;
    }
    //write command "Investigation"
    s_test_buffer_in[0] = 'C';
    att_handler(BLE_HANDLE_FILE_DESC, 0, s_test_buffer_in, sizeof(s_test_buffer_in), ATT_HANDLE_MODE_WRITE);

    TestReadBleFile(tc, FILE_NAME, 0);
}

CuSuite* BleHandlerTestGetSuite(void)
{
    CuSuite* suite = CuSuiteNew("BLE Handler Test");
    SUITE_ADD_TEST(suite, TestBleFile);
    SUITE_ADD_TEST(suite, TestBleSportsData);
    SUITE_ADD_TEST(suite, TestBleFile2);
    return suite;
}

