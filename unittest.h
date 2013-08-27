#define EXPECT_MSG(x, msg) \
	if (!(x)) {printf("%d: %s\n", __LINE__, msg); exit(-1);}

#define EXPECT(x) EXPECT_MSG(x, "expect " #x)
