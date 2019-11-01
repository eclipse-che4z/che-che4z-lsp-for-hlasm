#ifndef HLASMPARSER_PARSERLIBRARY_ANALYZER_H
#define HLASMPARSER_PARSERLIBRARY_ANALYZER_H

#include "context/hlasm_context.h"
#include "generated/hlasmparser.h"
#include "parser_error_listener.h"
#include "shared/token_stream.h"
#include "processing/processing_manager.h"
#include "processor.h"
#include "diagnosable_ctx.h"

namespace hlasm_plugin {
namespace parser_library {

//this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context 
class analyzer : public diagnosable_ctx
{
	context::ctx_ptr hlasm_ctx_;
	context::hlasm_context& hlasm_ctx_ref_;

	parser_error_listener listener_;

	semantics::lsp_info_processor lsp_proc_;

	input_source input_;
	lexer lexer_;
	token_stream tokens_;
	generated::hlasmparser* parser_;

	processing::processing_manager mngr_;
public:
	analyzer(const std::string& text, std::string file_name, context::hlasm_context& hlasm_ctx, parse_lib_provider& lib_provider, const library_data data);

	analyzer(const std::string& text,
		std::string file_name = "",
		parse_lib_provider& lib_provider = empty_parse_lib_provider::instance,
		processing::processing_tracer * tracer = nullptr);

	context::hlasm_context& context();
	generated::hlasmparser& parser();
	semantics::lsp_info_processor& lsp_processor();

	void analyze(std::atomic<bool>* cancel = nullptr);

	void collect_diags() const override;

private:
	analyzer(
		const std::string& text, 
		std::string file_name, 
		parse_lib_provider& lib_provider, 
		context::hlasm_context* hlasm_ctx, 
		const library_data data,
		bool own_ctx,
		processing::processing_tracer * tracer);

};

}
}
#endif
