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

namespace hlasm_plugin::parser_library::parsing {

parser_impl::parser_impl(antlr4::TokenStream* input)
    : Parser(input)
    , input(dynamic_cast<lexing::token_stream&>(*input))
    , hlasm_ctx(nullptr)
    , src_proc(nullptr)
    , processor(nullptr)
    , current_statement(nullptr)
    , finished_flag(false)
    , provider()
{}

void parser_impl::initialize(analyzing_context& a_ctx,
    semantics::source_info_processor* src_prc,
    workspaces::parse_lib_provider* lib_provider,
    processing::processing_state_listener* state_listener)
{
    ctx = &a_ctx;
    hlasm_ctx = ctx->hlasm_ctx.get();
    src_proc = src_prc;
    finished_flag = false;
    // TODO: remove lib_provider and state_listener arguments

    input_lexer = &dynamic_cast<lexing::lexer&>(*_input->getTokenSource());
}

bool parser_impl::is_last_line() const { return input_lexer->is_last_line(); }

context::source_position parser_impl::statement_start() const
{
    auto pos = input_lexer->last_lln_begin_position();
    return { pos.line, pos.offset };
}

context::source_position parser_impl::statement_end() const
{
    auto pos = input_lexer->last_lln_end_position();
    return { pos.line, pos.offset };
}

std::unique_ptr<parser_holder> parser_holder::create(
    semantics::source_info_processor* lsp_proc, performance_metrics* metrics)
{
    std::string s;
    auto h = std::make_unique<parser_holder>();
    h->input = std::make_unique<lexing::input_source>(s);
    h->lex = std::make_unique<lexing::lexer>(h->input.get(), lsp_proc, metrics);
    h->stream = std::make_unique<lexing::token_stream>(h->lex.get());
    h->parser = std::make_unique<hlasmparser>(h->stream.get());
    return h;
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
    return opcode.value == hlasm_ctx->ids().well_known.GBLA || opcode.value == hlasm_ctx->ids().well_known.GBLB
        || opcode.value == hlasm_ctx->ids().well_known.GBLC || opcode.value == hlasm_ctx->ids().well_known.LCLA
        || opcode.value == hlasm_ctx->ids().well_known.LCLB || opcode.value == hlasm_ctx->ids().well_known.LCLC;
}

self_def_t parser_impl::parse_self_def_term(const std::string& option, const std::string& value, range term_range)
{
    diagnostic_adder add_diagnostic(this, term_range);
    auto val = expressions::ca_constant::self_defining_term(option, value, add_diagnostic);

    if (add_diagnostic.diagnostics_present)
        diags().back().file_name = hlasm_ctx->processing_stack().back().proc_location.file;
    return val;
}

context::data_attr_kind parser_impl::get_attribute(std::string attr_data, range data_range)
{
    if (attr_data.size() == 1)
    {
        auto c = (char)std::toupper(attr_data[0]);
        return context::symbol_attributes::transform_attr(c);
    }

    add_diagnostic(
        diagnostic_s::error_S101(hlasm_ctx->processing_stack().back().proc_location.file, attr_data, data_range));

    return context::data_attr_kind::UNKNOWN;
}

context::id_index parser_impl::parse_identifier(std::string value, range id_range)
{
    if (value.size() > 63)
        add_diagnostic(
            diagnostic_s::error_S100(hlasm_ctx->processing_stack().back().proc_location.file, value, id_range));

    return hlasm_ctx->ids().add(std::move(value));
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const
{
    expr->resolve_expression_tree(type);
    expr->collect_diags();
    for (auto& d : expr->diags())
        add_diagnostic(diagnostic_s(hlasm_ctx->processing_stack().back().proc_location.file, std::move(d)));
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
    if (opcode.value == hlasm_ctx->ids().add("SETA") || opcode.value == hlasm_ctx->ids().add("ACTR")
        || opcode.value == hlasm_ctx->ids().add("ASPACE") || opcode.value == hlasm_ctx->ids().add("AGO"))
        resolve_expression(expr, context::SET_t_enum::A_TYPE);
    else if (opcode.value == hlasm_ctx->ids().add("SETB") || opcode.value == hlasm_ctx->ids().add("AIF"))
        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    else if (opcode.value == hlasm_ctx->ids().add("SETC"))
        resolve_expression(expr, context::SET_t_enum::C_TYPE);
    else if (opcode.value == hlasm_ctx->ids().add("AREAD"))
    {
        // aread operand is just enumeration
    }
    else
    {
        assert(false);
        resolve_expression(expr, context::SET_t_enum::UNDEF_TYPE);
    }
}

void parser_impl::set_source_indices(const antlr4::Token* start, const antlr4::Token* stop)
{
    assert(stop);
    size_t start_offset;

    if (start)
        start_offset = start->getStartIndex();
    else
        start_offset = stop->getStartIndex();
    // TODO: just keep hlasm_ctx???
    ctx->hlasm_ctx->set_source_indices(start_offset, stop->getStopIndex() + 1, stop->getLine());
}

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

void parser_impl::reinitialize(
    context::hlasm_context* h_ctx, semantics::range_provider range_prov, processing::processing_status proc_stat)
{
    hlasm_ctx = h_ctx;
    provider = range_prov;
    proc_status = proc_stat;
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
