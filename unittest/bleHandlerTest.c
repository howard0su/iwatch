
#include "CuTest.h"

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "TestUtility/stlv_test_stub.h"

#include "ble_handler.h"

#define BLE_HANDLE_SEED_BEGIN 0x000b
#define BLE_HANDLE_SEED_END   0x0037

uint16_t PickHandle(uint16_t index)
{
    return (BLE_HANDLE_SEED_BEGIN + index) * 2;
}

static void TestReadWriteInt32(CuTest* tc)
{
    uint32_t time = 0x12345678;
    CuAssertIntEquals(tc, 1, write_uint32(BLE_HANDLE_DATETIME, time));
    uint32_t readtime = read_uint32(BLE_HANDLE_DATETIME, 0);
    CuAssertIntEquals(tc, time, readtime);
}

static void TestReadWriteInt16(CuTest* tc)
{
    uint16_t data = 0x1234;
    CuAssertIntEquals(tc, 1, write_uint16(BLE_HANDLE_GPS_INFO, data));
    uint32_t readdata = read_uint16(BLE_HANDLE_GPS_INFO, 0);
    CuAssertIntEquals(tc, data, readdata);

    uint16_t datas[] = {0x1234, 0xabcd, 0x5678, 0x4321};
    CuAssertIntEquals(tc, 1, write_uint16_array(BLE_HANDLE_GPS_INFO, datas, 4));
    uint16_t* readret = read_uint16_array(BLE_HANDLE_GPS_INFO);
    CuAssertIntEquals(tc, 0, memcmp(datas, readret, 4 * 2));
}

static void TestReadWriteInt8(CuTest* tc)
{
    uint8_t data = 0x34;
    CuAssertIntEquals(tc, 1, write_uint8(BLE_HANDLE_ALARM_0, data));
    uint8_t readdata = read_uint8(BLE_HANDLE_ALARM_0, 0);
    CuAssertIntEquals(tc, data, readdata);

    uint8_t datas[] = {0x12, 0xab};
    CuAssertIntEquals(tc, 2, write_uint8_array(BLE_HANDLE_ALARM_0, datas, 2));
    uint8_t* readret = read_uint8_array(BLE_HANDLE_ALARM_0);
    CuAssertIntEquals(tc, 0, memcmp(datas, readret, 2));
}

static void TestReadWriteString(CuTest* tc)
{
    char str[] = "0123456789";
    CuAssertIntEquals(tc, sizeof(str), write_string(BLE_HANDLE_FILE_DESC, str));
    char* readret = read_string(BLE_HANDLE_FILE_DESC);
    CuAssertIntEquals(tc, 0, strcmp(str, readret));
}
CuSuite* BleHandlerTestGetSuite(void)
{
	CuSuite* suite = CuSuiteNew("BLE Handler Test");
    create_ble_handle_db();
    SUITE_ADD_TEST(suite, TestReadWriteInt32);
    SUITE_ADD_TEST(suite, TestReadWriteInt16);
    SUITE_ADD_TEST(suite, TestReadWriteInt8);
    SUITE_ADD_TEST(suite, TestReadWriteString);
    return suite;
}

