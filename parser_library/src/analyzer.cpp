#include "analyzer.h"
#include "error_strategy.h"

hlasm_plugin::parser_library::analyzer::analyzer(const std::string& text) : analyzer(text, std::make_shared<context::hlasm_context>()) {}

hlasm_plugin::parser_library::analyzer::analyzer(const std::string& text, context::ctx_ptr ctx) 
	:input_(text), lexer_(&input_), tokens_(&lexer_), parser_(&tokens_), ctx_(std::move(ctx)), mngr_(ctx_)
{
	parser_.setErrorHandler(std::make_shared<error_strategy>());

	parser_.initialize(&mngr_);
	mngr_.initialize(&parser_);

	parser_.removeErrorListeners();
	parser_.addErrorListener(&listener_);
}

hlasm_plugin::parser_library::context::ctx_ptr hlasm_plugin::parser_library::analyzer::context()
{
	return ctx_;
}

hlasm_plugin::parser_library::generated::hlasmparser & hlasm_plugin::parser_library::analyzer::parser()
{
	return parser_;
}

void hlasm_plugin::parser_library::analyzer::analyze()
{
	parser_.program();
}

void hlasm_plugin::parser_library::analyzer::collect_diags() const
{
	collect_diags_from_child(mngr_);
	collect_diags_from_child(listener_);
}