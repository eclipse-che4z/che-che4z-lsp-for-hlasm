#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSERIMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSERIMPL_H
#include "antlr4-runtime.h"
#include "../semantics/semantic_analyzer.h"
#include "../../include/shared/lexer.h"

#include "../semantics/expression.h"
#include "../semantics/arithmetic_expression.h"
#include "../semantics/logic_expression.h"
#include "../semantics/character_expression.h"
#include "../semantics/keyword_expression.h"

namespace hlasm_plugin {
namespace parser_library {

class parser_impl : public antlr4::Parser
{
public:
	semantics::semantic_analyzer analyzer;

	parser_impl(antlr4::TokenStream* input) : Parser(input) {}

	void enable_continuation();

	void disable_continuation();

	bool is_self_def();

	void initialize(std::shared_ptr<context::hlasm_context> ctx, lexer* lexer);

	void initialize(const semantics::semantic_analyzer& analyzer);

};

}
}
#endif
