#include "gmock/gmock.h"
#include "../include/shared/parser_library.h"
#include "context_test.h"

#include "workspace_test.h"



int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
