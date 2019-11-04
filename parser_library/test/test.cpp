#include "gmock/gmock.h"

#include "benchmark_test.h"

#include "data_definition_scale_test.h"
#include "data_definition_integer_test.h"
#include "debugger_test.h"

#include "data_definition_length_test.h"
#include "data_definition_check_test.h"
#include "data_definition_test.h"

#include "lsp_features_test.h"
#include "workspace_test.h"


#include "macro_test.h"
#include "copy_test.h"
#include "ord_sym_test.h"
#include "data_attribute_test.h"

#include "text_synchronization_test.h"
#include "asm_instr_check_test.h"
#include "mach_instr_check_test.h"
#include "lookahead_test.h"
//#include "parser_range_test.h"

#include "workspace_manager_test.h"

#include "parser_deferred_test.h"
#include "ca_instr_test.h"
#include "context_test.h"
#include "lexer_test.h"
#include "parser_test.h"

#include "utf_conv_test.h"
#include "diagnosable_ctx_test.h"

#include "diagnostics_check_test.h"
#include "asm_instr_diag_test.h"
#include "mach_instr_diag_test.h"

int main(int argc, char **argv)
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
