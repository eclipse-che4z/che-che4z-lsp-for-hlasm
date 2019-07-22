#include "gmock/gmock.h"

#include "workspace_folders_test.h"
#include "feature_text_synchronization_test.h"
#include "server_test.h"
#include "regress_test.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
