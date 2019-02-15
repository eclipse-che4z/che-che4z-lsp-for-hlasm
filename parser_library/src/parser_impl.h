#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H

#include <string>

#include "antlr4-runtime.h"

#include "semantics/semantic_analyzer.h"
#include "semantics/semantic_highlighting_info.h"
#include "shared/lexer.h"
#include "checking/instr_operand.h"

#include "context/hlasm_context.h"
#include "diagnosable_impl.h"

#include "semantics/expression.h"
#include "semantics/arithmetic_expression.h"
#include "semantics/logic_expression.h"
#include "semantics/character_expression.h"
#include "semantics/keyword_expression.h"
#include "semantics/semantic_info.h"

namespace hlasm_plugin {
namespace parser_library {

struct macro_def {
	std::string name = "";
	bool just_defined = false;
};

class parser_impl : public antlr4::Parser, public diagnosable_impl
{
public:
	semantics::semantic_info semantic_info;

	semantics::semantic_analyzer analyzer;

	parser_impl(antlr4::TokenStream* input) : Parser(input) {}

	void enable_continuation();

	void disable_continuation();

	semantics::symbol_occurence create_occurence(semantics::symbol_range range, std::string name, bool make_it_candidate = false);
	bool is_var_definition(const semantics::symbol_occurence & occ);
	bool is_seq_definition(const semantics::symbol_occurence & occ);
	void check_definition_candidates();
	macro_def current_macro_def;

	bool is_self_def();


	void initialize(std::shared_ptr<context::hlasm_context> ctx, lexer* lexer);


	void initialize(const semantics::semantic_analyzer& analyzer);


	void collect_diags() const override;
	std::string file_name;
	

	void add_diag(diagnostic_op diag);
	
	
};


}
}

#endif // !HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
