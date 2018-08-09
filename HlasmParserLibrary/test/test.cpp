#include "gmock/gmock.h"
#include "../include/shared/HlasmParserLibrary.h"

using namespace HlasmPlugin;
using namespace HlasmParserLibrary;

TEST(Hello, World)
{
	EXPECT_EQ(1, 1);
}


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
