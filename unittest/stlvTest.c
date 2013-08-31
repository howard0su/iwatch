
#include "CuTest.h"

#include "stlv.h"
#include "stlv_client.h"
#include "stlv_server.h"

void TestMsgHandler(CuTest* tc)
{
}

CuSuite* StlvProtocalGetSuite(void)
{
	CuSuite* suite = CuSuiteNew();
    SUITE_ADD_TEST(suite, TestMsgHandler);
    return suite;
}

