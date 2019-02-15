#include "gmock/gmock.h"
#include "../src/server.h"

#include "server_test.h"
#include "regress_test.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
