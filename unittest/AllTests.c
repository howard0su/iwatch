#include <stdio.h>

#include "CuTest.h"

CuSuite* cfsGetSuite(void);
CuSuite* obexGetSuite(void);
CuSuite* WindowGetSuite(void);
CuSuite* GestureGetSuite(void);
CuSuite* StlvProtocalGetSuite(void);

void RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew("iwatch");

	CuSuiteAddSuite(suite, cfsGetSuite());
	CuSuiteAddSuite(suite, obexGetSuite());
	CuSuiteAddSuite(suite, WindowGetSuite());
	CuSuiteAddSuite(suite, GestureGetSuite());
    CuSuiteAddSuite(suite, StlvProtocalGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
}

int main(void)
{
	RunAllTests();
}
