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

#include "parser_impl.h"

#include <cctype>

#include "error_strategy.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "parser_error_listener_ctx.h"
#include "processing/context_manager.h"
#include "processing/statement.h"

namespace hlasm_plugin::parser_library::parsing {

parser_impl::parser_impl(antlr4::TokenStream* input)
    : Parser(input)
    , input(dynamic_cast<lexing::token_stream&>(*input))
    , ctx(nullptr)
    , lsp_proc(nullptr)
    , processor(nullptr)
    , finished_flag(false)
    , provider()
{}

void parser_impl::initialize(context::hlasm_context* hlasm_ctx,
    semantics::lsp_info_processor* lsp_prc,
    workspaces::parse_lib_provider* lib_provider,
    processing::processing_state_listener* state_listener)
{
    ctx = hlasm_ctx;
    lsp_proc = lsp_prc;
    finished_flag = false;
    lib_provider_ = lib_provider;
    state_listener_ = state_listener;
}

bool parser_impl::is_last_line() const
{
    return dynamic_cast<lexing::lexer&>(*_input->getTokenSource()).is_last_line();
}

void parser_impl::rewind_input(context::source_position pos)
{
    finished_flag = false;
    _matchedEOF = false;
    input.rewind_input(lexing::lexer::stream_position { pos.file_line, pos.file_offset });
}

context::source_position parser_impl::statement_start() const
{
    auto pos = dynamic_cast<lexing::lexer&>(*_input->getTokenSource()).last_lln_begin_position();
    return { pos.line, pos.offset };
}

context::source_position parser_impl::statement_end() const
{
    auto pos = dynamic_cast<lexing::lexer&>(*_input->getTokenSource()).last_lln_end_position();
    return { pos.line, pos.offset };
}

std::unique_ptr<parser_holder> create_parser_holder()
{
    std::string s;
    auto h = std::make_unique<parser_holder>();
    h->input = std::make_unique<lexing::input_source>(s);
    h->lex = std::make_unique<lexing::lexer>(h->input.get(), nullptr);
    h->stream = std::make_unique<lexing::token_stream>(h->lex.get());
    h->parser = std::make_unique<hlasmparser>(h->stream.get());
    return h;
}

std::pair<semantics::operands_si, semantics::remarks_si> parser_impl::parse_operand_field(
    context::hlasm_context* hlasm_ctx,
    std::string field,
    bool after_substitution,
    semantics::range_provider field_range,
    processing::processing_status status)
{
    if (!rest_parser_)
        rest_parser_ = create_parser_holder();

    hlasm_ctx->metrics.reparsed_statements++;
    const parser_holder& h = *rest_parser_;

    std::optional<std::string> sub;
    if (after_substitution)
        sub = field;
    parser_error_listener_ctx listener(*hlasm_ctx, std::move(sub));

    h.input->reset(field);

    h.lex->reset();
    h.lex->set_file_offset(field_range.original_range.start);
    h.lex->set_unlimited_line(after_substitution);

    h.stream->reset();

    h.parser->initialize(hlasm_ctx, field_range, status);
    h.parser->setErrorHandler(std::make_shared<error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&listener);
    h.parser->reset();

    h.parser->collector.prepare_for_next_statement();

    semantics::op_rem line;
    auto& [format, opcode] = status;
    if (format.occurence == processing::operand_occurence::ABSENT
        || format.form == processing::processing_form::UNKNOWN)
        h.parser->op_rem_body_noop();
    else
    {
        switch (format.form)
        {
            case processing::processing_form::MAC:
                line = std::move(h.parser->op_rem_body_mac_r()->line);
                parse_macro_operands(line);
                break;
            case processing::processing_form::ASM:
                line = std::move(h.parser->op_rem_body_asm_r()->line);
                break;
            case processing::processing_form::MACH:
                line = std::move(h.parser->op_rem_body_mach_r()->line);
                break;
            case processing::processing_form::DAT:
                line = std::move(h.parser->op_rem_body_dat_r()->line);
                break;
            default:
                break;
        }
    }

    // indicates that the reparse reason is to resolve deferred operands (and not to substitute varsymbols)
    if (!after_substitution)
    {
        lsp_proc->process_lsp_symbols(h.parser->collector.extract_lsp_symbols(),
            ctx->ids().add(ctx->processing_stack().back().proc_location.file, true));
    }

    collect_diags_from_child(listener);

    for (size_t i = 0; i < line.operands.size(); i++)
    {
        if (!line.operands[i])
            line.operands[i] = std::make_unique<semantics::empty_operand>(field_range.original_range);
    }

    if (line.operands.size() == 1 && line.operands.front()->type == semantics::operand_type::EMPTY)
        line.operands.clear();

    if (after_substitution && line.operands.size() && line.operands.front()->type == semantics::operand_type::MODEL)
        line.operands.clear();

    range op_range = line.operands.empty()
        ? field_range.original_range
        : semantics::range_provider::union_range(
            line.operands.front()->operand_range, line.operands.back()->operand_range);
    range rem_range = line.remarks.empty()
        ? range(op_range.end)
        : semantics::range_provider::union_range(line.remarks.front(), line.remarks.back());

    return std::make_pair(semantics::operands_si(op_range, std::move(line.operands)),
        semantics::remarks_si(rem_range, std::move(line.remarks)));
}

void parser_impl::collect_diags() const
{
    if (rest_parser_)
        collect_diags_from_child(*rest_parser_->parser);
}

void parser_impl::enable_continuation() { input.enable_continuation(); }

void parser_impl::disable_continuation() { input.disable_continuation(); }

void parser_impl::enable_hidden() { input.enable_hidden(); }

void parser_impl::disable_hidden() { input.disable_hidden(); }

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

bool parser_impl::is_var_def()
{
    auto [_, opcode] = *proc_status;
    return opcode.value == ctx->ids().add("GBLA") || opcode.value == ctx->ids().add("GBLB")
        || opcode.value == ctx->ids().add("GBLC") || opcode.value == ctx->ids().add("LCLA")
        || opcode.value == ctx->ids().add("LCLB") || opcode.value == ctx->ids().add("LCLC");
}

self_def_t parser_impl::parse_self_def_term(const std::string& option, const std::string& value, range term_range)
{
    diagnostic_adder add_diagnostic(this, term_range);
    auto val = expressions::ca_constant::self_defining_term(option, value, add_diagnostic);

    if (add_diagnostic.diagnostics_present)
        diags().back().file_name = ctx->processing_stack().back().proc_location.file;
    return val;
}

context::data_attr_kind parser_impl::get_attribute(std::string attr_data, range data_range)
{
    if (attr_data.size() == 1)
    {
        auto c = (char)std::toupper(attr_data[0]);
        return context::symbol_attributes::transform_attr(c);
    }

    add_diagnostic(diagnostic_s::error_S101(ctx->processing_stack().back().proc_location.file, attr_data, data_range));

    return context::data_attr_kind::UNKNOWN;
}

context::id_index parser_impl::parse_identifier(std::string value, range id_range)
{
    if (value.size() > 63)
        add_diagnostic(diagnostic_s::error_S100(ctx->processing_stack().back().proc_location.file, value, id_range));

    return ctx->ids().add(std::move(value));
}

void parser_impl::parse_macro_operands(semantics::op_rem& line)
{
    if (line.operands.size())
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
        auto r = semantics::range_provider::union_range(
            line.operands.begin()->get()->operand_range, line.operands.back()->operand_range);

        line.operands = parse_macro_operands(std::move(to_parse), r, std::move(ranges));
    }
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const
{
    expr->resolve_expression_tree(type);
    expr->collect_diags();
    for (auto& d : expr->diags())
        add_diagnostic(diagnostic_s(ctx->processing_stack().back().proc_location.file, std::move(d)));
    expr->diags().clear();
}

void parser_impl::resolve_expression(std::vector<expressions::ca_expr_ptr>& expr_list, context::SET_t_enum type) const
{
    for (auto& expr : expr_list)
        resolve_expression(expr, type);
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr) const
{
    auto [_, opcode] = *proc_status;
    if (opcode.value == ctx->ids().add("SETA") || opcode.value == ctx->ids().add("ACTR")
        || opcode.value == ctx->ids().add("ASPACE") || opcode.value == ctx->ids().add("AGO"))
        resolve_expression(expr, context::SET_t_enum::A_TYPE);
    else if (opcode.value == ctx->ids().add("SETB") || opcode.value == ctx->ids().add("AIF"))
        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    else if (opcode.value == ctx->ids().add("SETC"))
        resolve_expression(expr, context::SET_t_enum::C_TYPE);
    else if (opcode.value == ctx->ids().add("AREAD"))
    {
        // aread operand is just enumeration
    }
    else
    {
        assert(false);
        resolve_expression(expr, context::SET_t_enum::UNDEF_TYPE);
    }
}

bool parser_impl::process_instruction()
{
    if (processor->kind == processing::processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(collector.current_instruction(), { *ctx, *lib_provider_ }, *state_listener_))
        return true;

    ctx->set_source_position(collector.current_instruction().field_range.start);
    proc_status = processor->get_processing_status(collector.peek_instruction());
    return false;
}

bool parser_impl::process_statement()
{
    bool hint = proc_status->first.form == processing::processing_form::DEFERRED;
    auto stmt(collector.extract_statement(hint, range(position(statement_start().file_line, 0))));
    context::unique_stmt_ptr ptr;

    range statement_range;
    if (!hint)
    {
        ptr = std::make_unique<processing::resolved_statement_impl>(
            std::move(std::get<semantics::statement_si>(stmt)), proc_status.value().second, proc_status.value().first);
        statement_range = dynamic_cast<processing::resolved_statement_impl*>(ptr.get())->stmt_range_ref();
    }
    else
    {
        assert(std::holds_alternative<semantics::statement_si_deferred>(stmt));
        ptr = std::make_unique<semantics::statement_si_deferred>(
            std::move(std::get<semantics::statement_si_deferred>(stmt)));
        statement_range = dynamic_cast<semantics::statement_si_deferred*>(ptr.get())->deferred_range_ref();
    }

    if (processor->kind == processing::processing_kind::ORDINARY
        && try_trigger_attribute_lookahead(*ptr, { *ctx, *lib_provider_ }, *state_listener_))
        return true;

    if (statement_range.start.line < statement_range.end.line)
        ctx->metrics.continued_statements++;
    else
        ctx->metrics.non_continued_statements++;

    lsp_proc->process_lsp_symbols(collector.extract_lsp_symbols());
    lsp_proc->process_hl_symbols(collector.extract_hl_symbols());
    collector.prepare_for_next_statement();

    processor->process_statement(std::move(ptr));

    return false;
}

void parser_impl::process_next(processing::statement_processor& proc)
{
    processor = &proc;

    if (proc.kind == processing::processing_kind::LOOKAHEAD)
        process_lookahead();
    else
        process_ordinary();

    processor = nullptr;
    collector.prepare_for_next_statement();
    proc_status.reset();
}

bool parser_impl::finished() const { return finished_flag; }

bool parser_impl::deferred()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::DEFERRED;
}

bool parser_impl::no_op()
{
    auto& [format, opcode] = *proc_status;
    return format.occurence == processing::operand_occurence::ABSENT;
}

bool parser_impl::ignored()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::IGNORED;
}

bool parser_impl::alt_format()
{
    auto& [format, opcode] = *proc_status;

    return format.form == processing::processing_form::CA || format.form == processing::processing_form::MAC;
}

bool parser_impl::MACH()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::MACH;
}

bool parser_impl::ASM()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::ASM;
}

bool parser_impl::DAT()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::DAT;
}

bool parser_impl::CA()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::CA;
}

bool parser_impl::MAC()
{
    auto& [format, opcode] = *proc_status;
    return format.form == processing::processing_form::MAC;
}

bool parser_impl::UNKNOWN()
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

semantics::operand_list parser_impl::parse_macro_operands(
    std::string operands, range field_range, std::vector<range> operand_ranges)
{
    if (!rest_parser_)
        rest_parser_ = create_parser_holder();

    const parser_holder& h = *rest_parser_;

    semantics::range_provider tmp_provider(field_range, operand_ranges, semantics::adjusting_state::MACRO_REPARSE);

    parser_error_listener_ctx listener(*ctx, std::nullopt, tmp_provider);

    h.input->reset(operands);

    h.lex->reset();
    h.lex->set_file_offset(field_range.start);
    h.lex->set_unlimited_line(true);

    h.stream->reset();

    h.parser->initialize(ctx, tmp_provider, *proc_status);
    h.parser->setErrorHandler(std::make_shared<error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&listener);

    h.parser->reset();

    h.parser->collector.prepare_for_next_statement();

    auto list = std::move(h.parser->macro_ops()->list);

    collect_diags_from_child(listener);

    return list;
}

void parser_impl::process_ordinary()
{
    auto lab_instr = dynamic_cast<hlasmparser&>(*this).lab_instr();

    if (!finished_flag && collector.has_instruction())
    {
        bool attr_look_needed = process_instruction();

        if (attr_look_needed)
            return;

        if (!lab_instr->op_text)
            process_statement();
        else
            parse_operands(std::move(*lab_instr->op_text), lab_instr->op_range);
    }
}

void parser_impl::process_lookahead()
{
    auto look_lab_instr = dynamic_cast<hlasmparser&>(*this).look_lab_instr();
    if (!finished_flag)
    {
        process_instruction();
        if (!look_lab_instr->op_text)
            process_statement();
        else
            parse_lookahead_operands(std::move(*look_lab_instr->op_text), look_lab_instr->op_range);
    }
}

void parser_impl::parse_operands(const std::string& text, range text_range)
{
    if (!rest_parser_)
        rest_parser_ = create_parser_holder();

    parser_holder& h = *rest_parser_;

    parser_error_listener_ctx listener(*ctx, std::nullopt);

    h.input->reset(text);

    h.lex->reset();
    h.lex->set_file_offset(text_range.start);
    h.lex->set_unlimited_line(false);

    h.stream->reset();

    h.parser->initialize(ctx, provider, *proc_status);
    h.parser->setErrorHandler(std::make_shared<error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&listener);

    h.parser->reset();

    h.parser->collector.prepare_for_next_statement();

    auto& [format, opcode] = *proc_status;
    if (format.occurence == processing::operand_occurence::ABSENT
        || format.form == processing::processing_form::UNKNOWN)
        h.parser->op_rem_body_noop();
    else
    {
        switch (format.form)
        {
            case processing::processing_form::IGNORED:
                h.parser->op_rem_body_ignored();
                break;
            case processing::processing_form::DEFERRED:
                h.parser->op_rem_body_deferred();
                break;
            case processing::processing_form::CA:
                h.parser->op_rem_body_ca();
                break;
            case processing::processing_form::MAC: {
                auto rule = h.parser->op_rem_body_mac();
                auto line = std::move(rule->line);
                auto line_range = rule->line_range;
                parse_macro_operands(line);
                h.parser->collector.set_operand_remark_field(
                    std::move(line.operands), std::move(line.remarks), line_range);
            }
            break;
            case processing::processing_form::ASM:
                h.parser->op_rem_body_asm();
                break;
            case processing::processing_form::MACH:
                h.parser->op_rem_body_mach();
                break;
            case processing::processing_form::DAT:
                h.parser->op_rem_body_dat();
                break;
            default:
                break;
        }
    }

    if (format.form != processing::processing_form::IGNORED)
    {
        collector.append_operand_field(std::move(h.parser->collector));
        process_statement();
    }

    collect_diags_from_child(listener);
}

void parser_impl::parse_lookahead_operands(const std::string& text, range text_range)
{
    if (!rest_parser_)
        rest_parser_ = create_parser_holder();

    if (proc_status->first.form == processing::processing_form::IGNORED)
    {
        process_statement();
        return;
    }

    // optimization : if statement has no label and is not COPY, do not even parse operands
    if (!collector.has_label())
    {
        if (collector.current_instruction().type == semantics::instruction_si_type::ORD)
        {
            context::id_index tmp;
            tmp = std::get<context::id_index>(collector.current_instruction().value);
            if (tmp != ctx->ids().add("COPY"))
            {
                process_statement();
                return;
            }
        }
    }

    const parser_holder& h = *rest_parser_;

    parser_error_listener_ctx listener(*ctx, std::nullopt);

    h.input->reset(text);

    h.lex->reset();
    h.lex->set_file_offset(text_range.start);
    h.lex->set_unlimited_line(true);

    h.stream->reset();

    h.parser->initialize(ctx, provider, *proc_status);
    h.parser->setErrorHandler(std::make_shared<error_strategy>());
    h.parser->removeErrorListeners();
    h.parser->addErrorListener(&listener);

    h.parser->reset();

    h.parser->collector.prepare_for_next_statement();

    h.parser->lookahead_operands_and_remarks();

    h.parser->collector.clear_hl_lsp_symbols();
    collector.append_operand_field(std::move(h.parser->collector));

    process_statement();
}

antlr4::misc::IntervalSet parser_impl::getExpectedTokens()
{
    if (proc_status->first.kind == processing::processing_kind::LOOKAHEAD)
        return {};
    else
        return antlr4::Parser::getExpectedTokens();
}

parser_holder::~parser_holder() {}

} // namespace hlasm_plugin::parser_library::parsing
