#ifndef HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H
#define HLASMPLUGIN_PARSERLIBRARY_PARSER_IMPL_H

#include "antlr4-runtime.h"
#include "diagnosable.h"
#include "context/hlasm_context.h"
#include "semantics/collector.h"
#include "semantics/lsp_info_processor.h"
#include "processing/statement_provider.h"
#include "processing/opencode_provider.h"
#include "processing/deferred_parser.h"

namespace hlasm_plugin {
namespace parser_library {

using self_def_t = std::int32_t;

struct parser_holder;

//class providing methods helpful for parsing and methods modifying parsing process
class parser_impl
	: public antlr4::Parser, public diagnosable_impl, 
	public processing::statement_provider , public processing::opencode_provider, public processing::statement_field_reparser
{
public:
	parser_impl(antlr4::TokenStream* input);

	void initialize(
		context::hlasm_context* hlasm_ctx, 
		semantics::lsp_info_processor * lsp_prc);

	bool is_last_line();
	virtual void rewind_input(context::opencode_sequence_symbol::opencode_position loc) override;
	virtual context::opencode_sequence_symbol::opencode_position statement_start() const override;
	virtual context::opencode_sequence_symbol::opencode_position statement_end() const override;

	virtual processing::statement_field_reparser::parse_result reparse_operand_field(
		context::hlasm_context* hlasm_ctx, 
		std::string field,semantics::range_provider field_range, 
		processing::processing_status status) override;

	void collect_diags() const override;

	std::vector<antlr4::ParserRuleContext*> tree;
protected:
	void enable_continuation();
	void disable_continuation();
	bool is_self_def();
	bool is_data_attr();
	bool is_var_def();
	self_def_t parse_self_def_term(const std::string& option, const std::string& value,range term_range);
	context::id_index parse_identifier(std::string value, range id_range);

	void process_instruction();
	void process_statement();

	virtual void process_next(processing::statement_processor& processor) override;
	virtual bool finished() const override;

	context::hlasm_context* ctx;
	semantics::lsp_info_processor* lsp_proc;
	processing::statement_processor* processor;
	std::optional<processing::processing_status> proc_status;
	bool finished_flag;
	semantics::collector collector;
	semantics::range_provider provider;

	bool deferred();
	bool no_op();
	bool alt_format();
	bool MACH();
	bool ASM();
	bool DAT();
	bool CA();
	bool MAC();
	bool UNKNOWN();

private:
	void initialize(context::hlasm_context* hlasm_ctx, semantics::range_provider range_prov, processing::processing_status proc_stat);

	std::vector<parser_holder> parsers_;

	bool last_line_processed_;
};


class input_source;
class lexer;
class token_stream;
namespace generated {
class hlasmparser;
}
struct parser_holder
{
	std::unique_ptr<input_source> input;
	std::unique_ptr<lexer> lex;
	std::unique_ptr<token_stream> stream;
	std::unique_ptr<generated::hlasmparser> parser;

	parser_holder(const parser_holder&) = delete;
	parser_holder(parser_holder&&);
	parser_holder()=default;

	~parser_holder();
};


}
}

#endif
