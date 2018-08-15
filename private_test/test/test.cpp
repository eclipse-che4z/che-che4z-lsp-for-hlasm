#include "gmock/gmock.h"
#include "../include/shared/parser_library.h"

using namespace HlasmPlugin;
using namespace HlasmParserLibrary;

TEST(PrivateTest, case1)
{
	EXPECT_EQ(1, 1);
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
