/*
 * Copyright (c) 2019 Broadcom.
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program and the accompanying materials are made
 * available under the terms of the Eclipse Public License 2.0
 * which is available at https://www.eclipse.org/legal/epl-2.0/
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Broadcom, Inc. - initial API and implementation
 */

#include <cctype>
#include "parser_impl.h"
#include "error_strategy.h"
#include "parser_error_listener_ctx.h"
#include "../include/shared/lexer.h"
#include "../include/shared/token_stream.h"
#include "generated/hlasmparser.h"
#include "expressions/arithmetic_expression.h"
#include "processing/statement.h"
#include "processing/context_manager.h"

using namespace hlasm_plugin::parser_library;

parser_impl::parser_impl(antlr4::TokenStream* input)
	: Parser(input),
	ctx(nullptr), lsp_proc(nullptr), processor(nullptr), finished_flag(false), provider(),
	last_line_processed_(false), line_end_pushed_(false) {}

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

void parser_impl::rewind_input(context::source_position pos)
{
	finished_flag = false;
	last_line_processed_ = false;
	_matchedEOF = false;
	dynamic_cast<token_stream&>(*_input).rewind_input(lexer::stream_position{ pos.file_line, pos.file_offset }, line_end_pushed_);
	line_end_pushed_ = false;
}

void parser_impl::push_line_end()
{
	line_end_pushed_ = dynamic_cast<token_stream&>(*_input).consume_EOLLN();
}

context::source_position parser_impl::statement_start() const
{
	auto pos = dynamic_cast<lexer&>(*_input->getTokenSource()).last_lln_begin_position();
	return { pos.line,pos.offset };
}

context::source_position parser_impl::statement_end() const
{
	auto pos = dynamic_cast<lexer&>(*_input->getTokenSource()).last_lln_end_position();
	return { pos.line,pos.offset };
}


std::pair<semantics::operands_si, semantics::remarks_si> parser_impl::parse_operand_field(
	context::hlasm_context* hlasm_ctx, std::string field, bool after_substitution, semantics::range_provider field_range, processing::processing_status status)
{
	hlasm_ctx->metrics.reparsed_statements++;
	parser_holder h;

	std::optional<std::string> sub;
	if (after_substitution)
		sub = field;
	parser_error_listener_ctx listener(*hlasm_ctx, std::move(sub));

	h.input = std::make_unique<input_source>(std::move(field));
	h.lex = std::make_unique<lexer>(h.input.get(), nullptr,&hlasm_ctx->metrics);
	h.stream = std::make_unique<token_stream>(h.lex.get());
	h.parser = std::make_unique<generated::hlasmparser>(h.stream.get());

	h.lex->set_file_offset(field_range.original_range.start);
	h.lex->set_unlimited_line(after_substitution);

	h.parser->initialize(hlasm_ctx, field_range, status);
	h.parser->setErrorHandler(std::make_shared<error_strategy>());
	h.parser->removeErrorListeners();
	h.parser->addErrorListener(&listener);

	auto line = std::move(h.parser->model_operands()->line);
	
	// indicates that the reparse is done to resolve deferred ordinary symbols (and not to substitute)
	if (!after_substitution)
	{
		lsp_proc->process_lsp_symbols(h.parser->collector.extract_lsp_symbols(), ctx->processing_stack().back().proc_location.file);
	}
	

	collect_diags_from_child(listener);

	parsers_.emplace_back(std::move(h));

	for (size_t i = 0; i < line.operands.size(); i++)
	{
		if (!line.operands[i])
			line.operands[i] = std::make_unique<semantics::empty_operand>(field_range.original_range);
	}

	range op_range = line.operands.empty() ?
		field_range.original_range :
		semantics::range_provider::union_range(line.operands.front()->operand_range, line.operands.back()->operand_range);
	range rem_range = line.remarks.empty() ?
		range(op_range.end) : 
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

void parser_impl::enable_hidden()
{
	dynamic_cast<token_stream&>(*_input).enable_hidden();
}

void parser_impl::disable_hidden()
{
	dynamic_cast<token_stream&>(*_input).disable_hidden();
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
	{
		ae->diag->diag_range = term_range;
		add_diagnostic(diagnostic_s(ctx->opencode_file_name(), *ae->diag));
	}
	else
		return ae->get_numeric_value();

	return 0;
}

context::data_attr_kind parser_impl::get_attribute(std::string attr_data, range data_range)
{
	if (attr_data.size() == 1)
	{
		auto c = (char)std::toupper(attr_data[0]);
		auto attr = context::symbol_attributes::transform_attr(c);
		if (context::symbol_attributes::ordinary_allowed(attr))
			return attr;
	}

	add_diagnostic(diagnostic_s::error_S101("", attr_data, data_range));

	return context::data_attr_kind::UNKNOWN;
}

context::id_index parser_impl::parse_identifier(std::string value, range id_range)
{
	if (value.size() > 63)
		add_diagnostic(diagnostic_s::error_S100("",value, id_range));

	return ctx->ids().add(std::move(value));
}

void parser_impl::parse_macro_operands(semantics::op_rem& line)
{
	if (line.operands.size() && MAC())
	{
		size_t string_size = line.operands.size();
		std::vector<range> ranges;

		for (auto& op : line.operands)
			if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(op.get()))
				string_size += m_op->value.size();

		std::string to_parse;
		to_parse.reserve(string_size);

		for (size_t i = 0; i < line.operands.size(); ++i)
		{
			if (auto m_op = dynamic_cast<semantics::macro_operand_string*>(line.operands[i].get()))
				to_parse.append(m_op->value);
			if (i != line.operands.size() - 1)
				to_parse.push_back(',');
			ranges.push_back(line.operands[i]->operand_range);
		}
		auto r = semantics::range_provider::union_range(line.operands.begin()->get()->operand_range, line.operands.back()->operand_range);

		line.operands = parse_macro_operands(std::move(to_parse), r, std::move(ranges));
	}
}

void parser_impl::process_instruction()
{
	ctx->set_source_position(collector.current_instruction().field_range.start);
	proc_status = processor->get_processing_status(collector.peek_instruction());
}

void parser_impl::process_statement()
{
	ctx->set_source_indices(statement_start().file_offset, statement_end().file_offset, statement_end().file_line);

	bool hint = proc_status->first.form == processing::processing_form::DEFERRED;
	auto stmt(collector.extract_statement(hint, range(position(statement_start().file_line, 0))));
	context::unique_stmt_ptr ptr;

	range statement_range;
	if (!hint)
	{
		ptr = std::make_unique< processing::resolved_statement_impl>(std::move(std::get<semantics::statement_si>(stmt)), proc_status.value().second);
		statement_range = dynamic_cast<processing::resolved_statement_impl*>(ptr.get())->stmt_range_ref();
	}
	else
	{
		assert(std::holds_alternative<semantics::statement_si_deferred>(stmt));
		ptr = std::make_unique< semantics::statement_si_deferred>(std::move(std::get<semantics::statement_si_deferred>(stmt)));
		statement_range = dynamic_cast<semantics::statement_si_deferred*>(ptr.get())->deferred_range_ref();
	}

	if (statement_range.start.line < statement_range.end.line)
		ctx->metrics.continued_statements++;
	else
		ctx->metrics.non_continued_statements++;
	
	lsp_proc->process_lsp_symbols(collector.extract_lsp_symbols());
	lsp_proc->process_hl_symbols(collector.extract_hl_symbols());
	collector.prepare_for_next_statement();

	processor->process_statement(std::move(ptr));
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

bool hlasm_plugin::parser_library::parser_impl::ignored()
{
	auto& [format, opcode] = *proc_status;
	return format.form == processing::processing_form::IGNORED;
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

void parser_impl::initialize(
	context::hlasm_context* hlasm_ctx, semantics::range_provider range_prov, processing::processing_status proc_stat)
{
	ctx = hlasm_ctx;
	provider = range_prov;
	proc_status = proc_stat;
}

semantics::operand_list parser_impl::parse_macro_operands(std::string operands, range field_range, std::vector<range> operand_ranges)
{
	parser_holder h;
	semantics::range_provider tmp_provider(field_range, operand_ranges, semantics::adjusting_state::MACRO_REPARSE);

	parser_error_listener_ctx listener(*ctx, std::nullopt, tmp_provider);

	h.input = std::make_unique<input_source>(std::move(operands));
	h.lex = std::make_unique<lexer>(h.input.get(), nullptr);
	h.stream = std::make_unique<token_stream>(h.lex.get());
	h.parser = std::make_unique<generated::hlasmparser>(h.stream.get());

	h.lex->set_file_offset(field_range.start);
	h.lex->set_unlimited_line(true);

	h.parser->initialize(ctx, tmp_provider, *proc_status);
	h.parser->setErrorHandler(std::make_shared<error_strategy>());
	h.parser->removeErrorListeners();
	h.parser->addErrorListener(&listener);

	auto list = std::move(h.parser->macro_ops()->list);

	collector.append_reparsed_symbols(std::move(h.parser->collector));

	collect_diags_from_child(listener);

	parsers_.emplace_back(std::move(h));

	return list;
}


parser_holder::parser_holder(parser_holder&& h) 
	:input(std::move(h.input)), lex(std::move(h.lex)), stream(std::move(h.stream)), parser(std::move(h.parser))
{
}

parser_holder::~parser_holder() {}