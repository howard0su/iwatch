#include <stdio.h>

#include "CuTest.h"

#include "btstack/ble/att.h"

att_connection_t connection;

void testatt(CuTest* tc)
{
  uint8_t data[256];
  uint8_t response[256];
  uint16_t respsize;
  uint8_t uuid128[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 1, 2, 3, 4, 5, 6};
  uint16_t requestsize = att_find_by_type_value_request(data, GATT_PRIMARY_SERVICE_UUID, 1, 0xffff, uuid128, 16);

  att_handle_request(&connection, data, requestsize, response);
}

CuSuite* AttTestGetSuite(void)
{
  CuSuite* suite = CuSuiteNew("cfs Test");

  SUITE_ADD_TEST(suite, testatt);

  return suite;
}
