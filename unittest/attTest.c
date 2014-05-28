#include <stdio.h>

#include "CuTest.h"

#include "btstack/ble/att.h"
#include "btstack/src/remote_device_db.h"
#include "btstack/ble/central_device_db.h"


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


void test_remote_db(CuTest* tc)
{
  link_key_type_t type = 1;
  bd_addr_t bd = {1, 2, 3, 4, 5, 6};
  link_key_t key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key, &type), 0);

  remote_device_db_memory.put_link_key(&bd, &key, type);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key, NULL), 1);

  // validate key
  for(int i = 0; i < 16; i++)
  {
    printf("%d ", key[i]);
    CuAssertTrue(tc, key[i] == i+1);
  }
  printf("\n");

  bd[0] = 0xff;
  key[0] = 0xff;
  remote_device_db_memory.put_link_key(&bd, &key, type);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key, &type) , 1);

  // validate key
  CuAssertIntEquals(tc, key[0], 0xff);
  for(int i = 1; i < 16; i++)
    CuAssertIntEquals(tc, key[i], i+1);

  CuAssertIntEquals(tc, type, 1);
  remote_device_db_memory.delete_link_key(&bd);

  CuAssertIntEquals(tc, remote_device_db_memory.get_link_key(&bd, &key, &type) ,  0);
}

void test_central_db(CuTest* tc)
{
  bd_addr_t bd = {1, 2, 3, 4, 5, 6};
  sm_key_t key = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};

  central_device_db_init();

  CuAssertIntEquals(tc, central_device_db_count(), 4);

  central_device_db_add(0, bd, key, key);

  CuAssertIntEquals(tc, central_device_db_count(), 4);

  int addr_type = 0xff;
  bd_addr_t addr;
  sm_key_t irk;
  
  central_device_db_info(0, &addr_type, addr, irk);

  CuAssertIntEquals(tc, addr_type, 0);
  CuAssertIntEquals(tc, addr[1], 2);

  central_device_db_add(0, bd, key, key);

  CuAssertIntEquals(tc, central_device_db_count(), 4);  
  central_device_db_info(1, &addr_type, addr, irk);

  CuAssertIntEquals(tc, addr_type, 0);
  CuAssertIntEquals(tc, addr[1], 2);

}
CuSuite* AttTestGetSuite(void)
{
  CuSuite* suite = CuSuiteNew("att");

  SUITE_ADD_TEST(suite, testatt);
  SUITE_ADD_TEST(suite, test_remote_db);
  SUITE_ADD_TEST(suite, test_central_db);


  return suite;
}
