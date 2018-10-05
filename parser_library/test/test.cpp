#include "gmock/gmock.h"
#include "lookahead_test.h"
#include "parser_range_test.h"
#include "parser_substitutions_test.h"
#include "ca_instr_test.h"
#include "context_test.h"
#include "lexer_test.h"
#include "parser_test.h"
#include "utf_conv_test.h"

#include "workspace_test.h"


int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
