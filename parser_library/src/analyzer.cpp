#include "analyzer.h"
#include "error_strategy.h"

namespace hlasm_plugin::parser_library
{

analyzer::analyzer(const std::string& text) : analyzer(text, std::make_shared<context::hlasm_context>(), empty_parse_lib_provider::instance, "") {}

analyzer::analyzer(const std::string& text, context::ctx_ptr ctx, parse_lib_provider & lib_provider, std::string file_name)
	: diagnosable_ctx(ctx),
	ctx_(ctx), listener_(file_name), input_(text), lexer_(&input_), tokens_(&lexer_), parser_(&tokens_), mngr_(std::move(file_name), std::move(ctx), lib_provider)
{
	parser_.setErrorHandler(std::make_shared<error_strategy>());

	parser_.initialize(&mngr_);
	mngr_.initialize(&parser_);

	parser_.removeErrorListeners();
	parser_.addErrorListener(&listener_);
}

context::ctx_ptr analyzer::context()
{
	return ctx_;
}

generated::hlasmparser & analyzer::parser()
{
	return parser_;
}

void analyzer::analyze()
{
	parser_.program();
}

void analyzer::collect_diags() const
{
	collect_diags_from_child(mngr_);
	collect_diags_from_child(listener_);
}

}
