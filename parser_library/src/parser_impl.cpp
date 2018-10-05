#include "parser_impl.h"
#include "../include/shared/token_stream.h"

using namespace hlasm_plugin::parser_library;

void parser_impl::enable_continuation()
{
	dynamic_cast<token_stream*>(_input)->enable_continuation();
}

void parser_impl::disable_continuation()
{
	dynamic_cast<token_stream*>(_input)->disable_continuation();
}

bool parser_impl::is_self_def()
{
	std::string tmp(_input->LT(1)->getText());
	hlasm_plugin::parser_library::context::to_upper(tmp);
	return tmp == "B" || tmp == "X" || tmp == "C" || tmp == "G";
}

void parser_impl::initialize(std::shared_ptr<context::hlasm_context> ctx, lexer* lexer)
{
	analyzer.initialize(std::move(ctx), lexer);
}

void parser_impl::initialize(const semantics::semantic_analyzer& analyzer_init)
{
	this->analyzer.initialize(analyzer_init);
}
