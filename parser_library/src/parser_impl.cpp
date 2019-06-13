#include "parser_impl.h"
#include "../include/shared/lexer.h"
#include "../include/shared/token_stream.h"
#include "generated/hlasmparser.h"
#include "semantics/processing_manager.h"

using namespace hlasm_plugin::parser_library;

hlasm_plugin::parser_library::parser_impl::parser_impl(antlr4::TokenStream * input) : Parser(input) {}

bool hlasm_plugin::parser_library::parser_impl::is_last_line()
{
	return dynamic_cast<lexer*>(_input->getTokenSource())->is_last_line();
}

void hlasm_plugin::parser_library::parser_impl::rewind_input(hlasm_plugin::parser_library::location location)
{
	dynamic_cast<lexer*>(_input->getTokenSource())->rewind_input(location);
}

semantics::operand_remark_semantic_info hlasm_plugin::parser_library::parser_impl::reparse_operand_remark_field(std::string field)
{
	//todo set correct ranges, propagate errors
	semantics::operand_remark_semantic_info ret;

	input_source input(std::move(field));
	hlasm_plugin::parser_library::lexer lex(&input,lsp_proc);
	lex.set_unlimited_line(true);
	token_stream tokens(&lex);
	generated::hlasmparser operand_parser(&tokens);
	operand_parser.format = format;
	operand_parser.removeErrorListeners();
	auto res = operand_parser.operands_model();
	ret.operands = std::move(res->line.operands);
	ret.remarks = std::move(res->line.remarks);

	return ret;
}

hlasm_plugin::parser_library::location hlasm_plugin::parser_library::parser_impl::statement_start() const
{
	return dynamic_cast<lexer*>(_input->getTokenSource())->last_lln_begin_location();
}

void hlasm_plugin::parser_library::parser_impl::initialize(semantics::processing_manager* proc_mngr,semantics::lsp_info_processor* lsp_prc)
{
	mngr = proc_mngr;
	lsp_proc = lsp_prc;
}

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

void hlasm_plugin::parser_library::parser_impl::process_instruction()
{
	mngr->process_instruction(collector.extract_instruction_field());
}

void hlasm_plugin::parser_library::parser_impl::process_statement()
{
	mngr->process_statement(collector.extract_statement());
	lsp_proc->process_lsp_symbols(collector.extract_lsp_symbols());
	lsp_proc->process_hl_symbols(collector.extract_hl_symbols());
	collector.prepare_for_next_statement();
}
