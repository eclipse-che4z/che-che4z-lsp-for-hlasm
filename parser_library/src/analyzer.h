#ifndef HLASMPARSER_PARSERLIBRARY_ANALYZER_H
#define HLASMPARSER_PARSERLIBRARY_ANALYZER_H

#include "context/hlasm_context.h"
#include "generated/hlasmparser.h"
#include "parser_error_listener.h"
#include "shared/token_stream.h"
#include "semantics/processing_manager.h"
#include "processor.h"
#include "diagnosable_ctx.h"

namespace hlasm_plugin {
namespace parser_library {

//this class analyzes provided text and produces diagnostics and highlighting info with respect to provided context 
class analyzer : public diagnosable_ctx
{
	context::ctx_ptr ctx_;

	parser_error_listener listener_;

	input_source input_;
	lexer lexer_;
	token_stream tokens_;
	generated::hlasmparser parser_;

	semantics::processing_manager mngr_;

public:
	analyzer(const std::string& text);
	analyzer(const std::string& text, std::string file_name);
	analyzer(const std::string& text, context::ctx_ptr ctx, parse_lib_provider & lib_provider, std::string file_name);

	context::ctx_ptr context();
	generated::hlasmparser& parser();

	void analyze();

	void collect_diags() const override;
};

}
}
#endif
