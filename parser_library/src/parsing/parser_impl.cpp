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

#include "context/literal_pool.h"
#include "error_strategy.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "hlasmparser.h"
#include "lexing/token_stream.h"
#include "processing/context_manager.h"

namespace hlasm_plugin::parser_library::parsing {

parser_impl::parser_impl(antlr4::TokenStream* input)
    : Parser(input)
    , input(dynamic_cast<lexing::token_stream&>(*input))
    , hlasm_ctx(nullptr)
    , provider()
    , err_listener_(&provider)
{}

void parser_impl::initialize(context::hlasm_context* hl_ctx, diagnostic_op_consumer* d)
{
    removeErrorListeners();
    addErrorListener(&err_listener_);

    hlasm_ctx = hl_ctx;
    diagnoser_ = d;
    err_listener_.diagnoser = d;
}

void parser_impl::reinitialize(context::hlasm_context* h_ctx,
    semantics::range_provider range_prov,
    processing::processing_status proc_stat,
    diagnostic_op_consumer* d)
{
    hlasm_ctx = h_ctx;
    provider = std::move(range_prov);
    proc_status = proc_stat;
    diagnoser_ = d;
    err_listener_.diagnoser = d;
}

std::unique_ptr<parser_holder> parser_holder::create(
    semantics::source_info_processor* lsp_proc, context::hlasm_context* hl_ctx, diagnostic_op_consumer* d)
{
    std::string s;
    auto h = std::make_unique<parser_holder>();
    h->error_handler = std::make_shared<parsing::error_strategy>();
    h->input = std::make_unique<lexing::input_source>(s);
    h->lex = std::make_unique<lexing::lexer>(h->input.get(), lsp_proc);
    h->stream = std::make_unique<lexing::token_stream>(h->lex.get());
    h->parser = std::make_unique<hlasmparser>(h->stream.get());
    h->parser->setErrorHandler(h->error_handler);
    h->parser->initialize(hl_ctx, d);
    return h;
}

void parser_impl::enable_continuation() { input.enable_continuation(); }

void parser_impl::disable_continuation() { input.disable_continuation(); }

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
    const auto& wk = hlasm_ctx->ids().well_known;
    return opcode.value == wk.GBLA || opcode.value == wk.GBLB || opcode.value == wk.GBLC || opcode.value == wk.LCLA
        || opcode.value == wk.LCLB || opcode.value == wk.LCLC;
}

self_def_t parser_impl::parse_self_def_term(const std::string& option, const std::string& value, range term_range)
{
    auto add_diagnostic = diagnoser_ ? diagnostic_adder(*diagnoser_, term_range) : diagnostic_adder(term_range);
    return expressions::ca_constant::self_defining_term(option, value, add_diagnostic);
}

context::data_attr_kind parser_impl::get_attribute(std::string attr_data)
{
    // This function is called only from grammar when there are tokens ORDSYMOL ATTR
    // ATTR is not generated by lexer unless the ordsymbol token has length 1
    auto c = (char)std::toupper((unsigned char)attr_data[0]);
    return context::symbol_attributes::transform_attr(c);
}

context::id_index parser_impl::parse_identifier(std::string value, range id_range)
{
    if (value.size() > 63 && diagnoser_)
        diagnoser_->add_diagnostic(diagnostic_op::error_S100(value, id_range));

    return hlasm_ctx->ids().add(std::move(value));
}

context::id_index parser_impl::add_literal(std::string literal_text, expressions::data_definition dd, range)
{
    return hlasm_ctx->literals().add_literal(std::move(literal_text), std::move(dd));
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const
{
    expr->resolve_expression_tree(type);
    expr->collect_diags();
    if (diagnoser_)
        for (auto& d : expr->diags())
            diagnoser_->add_diagnostic(std::move(d));
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
    const auto& wk = hlasm_ctx->ids().well_known;
    if (opcode.value == wk.SETA || opcode.value == wk.ACTR || opcode.value == wk.ASPACE || opcode.value == wk.AGO
        || opcode.value == wk.MHELP)
        resolve_expression(expr, context::SET_t_enum::A_TYPE);
    else if (opcode.value == wk.SETB)
    {
        if (!expr->is_compatible(ca_expression_compatibility::setb))
            expr->add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    }
    else if (opcode.value == wk.AIF)
    {
        if (!expr->is_compatible(ca_expression_compatibility::aif))
            expr->add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    }
    else if (opcode.value == wk.SETC)
    {
        resolve_expression(expr, context::SET_t_enum::C_TYPE);
        if (!expr->is_character_expression(character_expression_purpose::assignment) && diagnoser_)
            diagnoser_->add_diagnostic(diagnostic_op::error_CE017_character_expression_expected(expr->expr_range));
    }
    else if (opcode.value == wk.AREAD)
    {
        // aread operand is just enumeration
    }
    else
    {
        assert(false);
        resolve_expression(expr, context::SET_t_enum::UNDEF_TYPE);
    }
}

bool parser_impl::MACH()
{
    auto& [format, _] = *proc_status;
    return format.form == processing::processing_form::MACH;
}

bool parser_impl::ASM()
{
    auto& [format, _] = *proc_status;
    return format.form == processing::processing_form::ASM;
}

bool parser_impl::DAT()
{
    auto& [format, _] = *proc_status;
    return format.form == processing::processing_form::DAT;
}

bool parser_impl::ALIAS()
{
    auto& [_, opcode] = *proc_status;
    return opcode.type == instruction_type::ASM && opcode.value == hlasm_ctx->ids().well_known.ALIAS;
}

antlr4::misc::IntervalSet parser_impl::getExpectedTokens()
{
    if (proc_status->first.kind == processing::processing_kind::LOOKAHEAD)
        return {};
    else
        return antlr4::Parser::getExpectedTokens();
}

parser_holder::~parser_holder() = default;

} // namespace hlasm_plugin::parser_library::parsing
