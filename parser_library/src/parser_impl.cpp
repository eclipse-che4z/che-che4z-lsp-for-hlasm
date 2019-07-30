#include "parser_impl.h"
#include "../include/shared/lexer.h"
#include "../include/shared/token_stream.h"
#include "generated/hlasmparser.h"
#include "expressions/arithmetic_expression.h"
#include "processing/statement.h"
#include "processing/context_manager.h"

using namespace hlasm_plugin::parser_library;

parser_impl::parser_impl(antlr4::TokenStream* input)
	: Parser(input), processing::statement_provider(processing::statement_provider_kind::OPEN),
	ctx(nullptr),lsp_proc(nullptr), processor(nullptr), finished_flag(false),provider(range()), last_line_processed_(false){}

void parser_impl::initialize(
	context::hlasm_context* hlasm_ctx,
	semantics::lsp_info_processor* lsp_prc)
{
	ctx = hlasm_ctx;
	lsp_proc = lsp_prc;
	finished_flag = false;
}

bool parser_impl::is_last_line()
{
	return dynamic_cast<lexer&>(*_input->getTokenSource()).is_last_line();
}

void parser_impl::rewind_input(context::opencode_sequence_symbol::opencode_position loc)
{
	finished_flag = false;
	last_line_processed_ = false;
	dynamic_cast<token_stream&>(*_input).rewind_input(lexer::stream_position{ loc.file_line, loc.file_offset });
}

context::opencode_sequence_symbol::opencode_position parser_impl::statement_start() const
{
	auto pos = dynamic_cast<lexer&>(*_input->getTokenSource()).last_lln_begin_position();
	return { pos.line,pos.offset };
}


std::pair<semantics::operands_si, semantics::remarks_si> parser_impl::reparse_operand_field(
	context::hlasm_context* hlasm_ctx, std::string field, semantics::range_provider field_range, processing::processing_status status)
{
	parser_holder h;
	h.input = std::make_unique<input_source>(std::move(field));
	h.lex = std::make_unique<lexer>(h.input.get(), nullptr);
	h.stream = std::make_unique<token_stream>(h.lex.get());
	h.parser = std::make_unique<generated::hlasmparser>(h.stream.get());

	h.parser->initialize(hlasm_ctx, field_range, status);
	h.parser->removeErrorListeners();

	auto line = std::move(h.parser->model_operands()->line);

	parsers_.emplace_back(std::move(h));

	for (size_t i = 0; i < line.operands.size(); i++)
	{
		if (!line.operands[i])
			line.operands[i] = std::make_unique<semantics::undefined_operand>(field_range.original_range);
	}

	range op_range = line.operands.empty() ?
		field_range.original_range :
		semantics::range_provider::union_range(line.operands.front()->operand_range, line.operands.back()->operand_range);
	range rem_range = line.remarks.empty() ?
		op_range : 
		semantics::range_provider::union_range(line.remarks.front(), line.remarks.back());

	return std::make_pair(
		semantics::operands_si(op_range,std::move(line.operands)),
		semantics::remarks_si(rem_range,std::move(line.remarks))
	);
}

void parser_impl::collect_diags() const {}




void parser_impl::enable_continuation()
{
	dynamic_cast<token_stream&>(*_input).enable_continuation();
}

void parser_impl::disable_continuation()
{
	dynamic_cast<token_stream&>(*_input).disable_continuation();
}

bool parser_impl::is_self_def()
{
	std::string tmp(_input->LT(1)->getText());
	context::to_upper(tmp);
	return tmp == "B" || tmp == "X" || tmp == "C" || tmp == "G";
}

bool parser_impl::is_data_attr()
{
	std::string tmp(_input->LT(1)->getText());
	context::to_upper(tmp);
	return tmp == "D" || tmp == "O" || tmp == "N" || tmp == "S" || tmp == "K" || tmp == "I" || tmp == "L" || tmp == "T";
}

bool hlasm_plugin::parser_library::parser_impl::is_var_def()
{
	auto [_,opcode] = *proc_status;
	return opcode.value == ctx->ids().add("GBLA") || opcode.value == ctx->ids().add("GBLB") || opcode.value == ctx->ids().add("GBLC") ||
		opcode.value == ctx->ids().add("LCLA") || opcode.value == ctx->ids().add("LCLB") || opcode.value == ctx->ids().add("LCLC");
}

self_def_t parser_impl::parse_self_def_term(const std::string& option, const std::string& value, range term_range)
{
	auto ae = expressions::arithmetic_expression::from_string(option, value, false); //could generate diagnostic + DBCS
	if (ae->has_error())
		add_diagnostic({ term_range, *ae->diag });
	else
		return ae->get_numeric_value();

	return 0;
}

context::id_index parser_impl::parse_identifier(std::string value, range id_range)
{
	if (value.size() > 63)
		add_diagnostic(diagnostic_s::error_E041("",value, id_range));

	return ctx->ids().add(std::move(value));
}



void parser_impl::process_instruction()
{
	ctx->set_file_position(collector.current_instruction().field_range.start);
	proc_status = processor->get_processing_status(collector.peek_instruction());
}

void parser_impl::process_statement()
{
	bool hint = proc_status->first.form == processing::processing_form::DEFERRED;
	auto stmt(collector.extract_statement(hint));

	context::unique_stmt_ptr ptr;

	if (!hint)
	{
		ptr = std::make_unique< processing::resolved_statement_impl>(std::move(std::get<semantics::statement_si>(stmt)), proc_status.value().second);
	}
	else
	{
		assert(std::holds_alternative<semantics::statement_si_deferred>(stmt));
		ptr = std::make_unique< semantics::statement_si_deferred>(std::move(std::get<semantics::statement_si_deferred>(stmt)));
	}

	processor->process_statement(std::move(ptr));

	lsp_proc->process_lsp_symbols(collector.extract_lsp_symbols());
	lsp_proc->process_hl_symbols(collector.extract_hl_symbols());
	collector.prepare_for_next_statement();
}

void parser_impl::process_next(processing::statement_processor& proc)
{
	finished_flag = last_line_processed_ && is_last_line();
	last_line_processed_ = is_last_line();
	processor = &proc;
	tree.push_back(dynamic_cast<generated::hlasmparser&>(*this).program_line());
	processor = nullptr;
	proc_status.reset();
}

bool parser_impl::finished() const
{
	return finished_flag;
}

bool hlasm_plugin::parser_library::parser_impl::deferred()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::DEFERRED;
}

bool hlasm_plugin::parser_library::parser_impl::no_op()
{
	auto& [format, opcode] = *proc_status;
	return format.occurence == processing::operand_occurence::ABSENT;
}

bool hlasm_plugin::parser_library::parser_impl::alt_format()
{
	auto& [format, opcode] = *proc_status;

	return format.form == processing::processing_form::CA || format.form == processing::processing_form::MAC;
}

bool hlasm_plugin::parser_library::parser_impl::MACH()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::MACH;
}

bool hlasm_plugin::parser_library::parser_impl::ASM()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::ASM;
}

bool hlasm_plugin::parser_library::parser_impl::DAT()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::DAT;
}

bool hlasm_plugin::parser_library::parser_impl::CA()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::CA;
}

bool hlasm_plugin::parser_library::parser_impl::MAC()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::MAC;
}

bool hlasm_plugin::parser_library::parser_impl::UNKNOWN()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::UNKNOWN;
}

void parser_impl::initialize(context::hlasm_context* hlasm_ctx, semantics::range_provider range_prov, processing::processing_status proc_stat)
{
	ctx = hlasm_ctx;
	provider = range_prov;
	proc_status = proc_stat;
}


parser_holder::parser_holder(parser_holder&& h) 
	:input(std::move(h.input)), lex(std::move(h.lex)), stream(std::move(h.stream)), parser(std::move(h.parser))
{
}

parser_holder::~parser_holder() {}