#include "analyzer.h"
#include "error_strategy.h"

using namespace hlasm_plugin::parser_library;

analyzer::analyzer(const std::string& text, std::string file_name, parse_lib_provider& lib_provider, context::hlasm_context* hlasm_ctx, bool own_ctx)
	:diagnosable_ctx(*hlasm_ctx),
	hlasm_ctx_(own_ctx ? context::ctx_ptr(hlasm_ctx) : nullptr), hlasm_ctx_ref_(*hlasm_ctx),
	listener_(file_name), 
	lsp_proc_(file_name, text, hlasm_ctx_ref_.lsp_ctx),
	input_(text), lexer_(&input_, &lsp_proc_), tokens_(&lexer_), parser_(new generated::hlasmparser(&tokens_)),
	mngr_(processing::provider_ptr(parser_), hlasm_ctx_ref_, lib_provider,*parser_)
{
	parser_->initialize(&hlasm_ctx_ref_, &lsp_proc_);
	parser_->setErrorHandler(std::make_shared<error_strategy>());
	parser_->removeErrorListeners();
	parser_->addErrorListener(&listener_);
}

analyzer::analyzer(const std::string& text, const std::string file_name, context::hlasm_context& hlasm_ctx)
	: analyzer(text, file_name, empty_parse_lib_provider::instance, &hlasm_ctx, false) 
{
	hlasm_ctx.push_processing_file(std::move(file_name), context::file_processing_type::MACRO);
}

analyzer::analyzer(const std::string& text, const std::string file_name, parse_lib_provider& lib_provider)
	: analyzer(text, std::move(file_name), lib_provider, new context::hlasm_context(file_name), true) {}

context::hlasm_context& analyzer::context()
{
	return hlasm_ctx_ref_;
}

generated::hlasmparser& analyzer::parser()
{
	return *parser_;
}

semantics::lsp_info_processor& analyzer::lsp_processor()
{
	return lsp_proc_;
}

void analyzer::analyze()
{
	mngr_.start_processing();
}

void analyzer::collect_diags() const
{
	collect_diags_from_child(mngr_);
	collect_diags_from_child(listener_);
}

