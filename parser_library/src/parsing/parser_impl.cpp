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

#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "error_strategy.h"
#include "expressions/conditional_assembly/ca_expr_visitor.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "hlasmparser_multiline.h"
#include "hlasmparser_singleline.h"
#include "lexing/token_stream.h"
#include "processing/op_code.h"

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

template<bool multiline>
struct parser_holder_impl final : parser_holder
{
    using parser_t = std::conditional_t<multiline, hlasmparser_multiline, hlasmparser_singleline>;
    parser_holder_impl(
        semantics::source_info_processor* lsp_proc, context::hlasm_context* hl_ctx, diagnostic_op_consumer* d)
    {
        error_handler = std::make_shared<parsing::error_strategy>();
        input = std::make_unique<lexing::input_source>(std::string());
        lex = std::make_unique<lexing::lexer>(input.get(), lsp_proc);
        stream = std::make_unique<lexing::token_stream>(lex.get());
        parser = std::make_unique<parser_t>(stream.get());
        parser->setErrorHandler(error_handler);
        parser->initialize(hl_ctx, d);
    }
    auto& get_parser() const { return static_cast<parser_t&>(*parser); }

    std::pair<std::optional<std::string>, range> lab_instr() const override
    {
        auto rule = get_parser().lab_instr();
        return { std::move(rule->op_text), rule->op_range };
    }
    std::pair<std::optional<std::string>, range> look_lab_instr() const override
    {
        auto rule = get_parser().look_lab_instr();
        return { std::move(rule->op_text), rule->op_range };
    }

    void op_rem_body_noop() const override { get_parser().op_rem_body_noop(); }
    void op_rem_body_ignored() const override { get_parser().op_rem_body_ignored(); }
    void op_rem_body_deferred() const override { get_parser().op_rem_body_deferred(); }
    void lookahead_operands_and_remarks() const override { get_parser().lookahead_operands_and_remarks(); }

    semantics::op_rem op_rem_body_mac_r() const override { return std::move(get_parser().op_rem_body_mac_r()->line); }
    semantics::operand_list macro_ops() const override { return std::move(get_parser().macro_ops()->list); }
    semantics::op_rem op_rem_body_asm_r() const override { return std::move(get_parser().op_rem_body_asm_r()->line); }
    semantics::op_rem op_rem_body_mach_r() const override { return std::move(get_parser().op_rem_body_mach_r()->line); }
    semantics::op_rem op_rem_body_dat_r() const override { return std::move(get_parser().op_rem_body_dat_r()->line); }


    void op_rem_body_ca_expr() const override { get_parser().op_rem_body_ca_expr(); }
    void op_rem_body_ca_branch() const override { get_parser().op_rem_body_ca_branch(); }
    void op_rem_body_ca_var_def() const override { get_parser().op_rem_body_ca_var_def(); }

    void op_rem_body_dat() const override { get_parser().op_rem_body_dat(); }
    void op_rem_body_mach() const override { get_parser().op_rem_body_mach(); }
    void op_rem_body_asm() const override { get_parser().op_rem_body_asm(); }

    std::pair<semantics::op_rem, range> op_rem_body_mac() const override
    {
        auto rule = get_parser().op_rem_body_mac();
        return { std::move(rule->line), rule->line_range };
    }

    semantics::literal_si literal_reparse() const override { return std::move(get_parser().literal_reparse()->value); }
};

std::unique_ptr<parser_holder> parser_holder::create(semantics::source_info_processor* lsp_proc,
    context::hlasm_context* hl_ctx,
    diagnostic_op_consumer* d,
    bool multiline)
{
    if (multiline)
        return std::make_unique<parser_holder_impl<true>>(lsp_proc, hl_ctx, d);
    else
        return std::make_unique<parser_holder_impl<false>>(lsp_proc, hl_ctx, d);
}

void parser_impl::enable_continuation() { input.enable_continuation(); }

void parser_impl::disable_continuation() { input.disable_continuation(); }

bool parser_impl::is_self_def()
{
    std::string tmp(_input->LT(1)->getText());
    context::to_upper(tmp);
    return tmp == "B" || tmp == "X" || tmp == "C" || tmp == "G";
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

size_t parser_impl::get_loctr_len() const
{
    auto [_, opcode] = *proc_status;
    return processing::processing_status_cache_key::generate_loctr_len(opcode.value);
}

bool parser_impl::loctr_len_allowed(const std::string& attr) const
{
    return (attr == "L" || attr == "l") && proc_status.has_value();
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr, context::SET_t_enum type) const
{
    diagnostic_consumer_transform diags([this](diagnostic_op d) {
        if (diagnoser_)
            diagnoser_->add_diagnostic(std::move(d));
    });
    expr->resolve_expression_tree(type, diags);
}

void parser_impl::resolve_expression(std::vector<expressions::ca_expr_ptr>& expr_list, context::SET_t_enum type) const
{
    for (auto& expr : expr_list)
        resolve_expression(expr, type);
}

void parser_impl::resolve_expression(expressions::ca_expr_ptr& expr) const
{
    diagnostic_consumer_transform diags([this](diagnostic_op d) {
        if (diagnoser_)
            diagnoser_->add_diagnostic(std::move(d));
    });
    auto [_, opcode] = *proc_status;
    const auto& wk = hlasm_ctx->ids().well_known;
    if (opcode.value == wk.SETA || opcode.value == wk.ACTR || opcode.value == wk.ASPACE || opcode.value == wk.AGO
        || opcode.value == wk.MHELP)
        resolve_expression(expr, context::SET_t_enum::A_TYPE);
    else if (opcode.value == wk.SETB)
    {
        if (!expr->is_compatible(ca_expression_compatibility::setb))
            diags.add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    }
    else if (opcode.value == wk.AIF)
    {
        if (!expr->is_compatible(ca_expression_compatibility::aif))
            diags.add_diagnostic(diagnostic_op::error_CE016_logical_expression_parenthesis(expr->expr_range));

        resolve_expression(expr, context::SET_t_enum::B_TYPE);
    }
    else if (opcode.value == wk.SETC)
    {
        resolve_expression(expr, context::SET_t_enum::C_TYPE);
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

void parser_impl::resolve_concat_chain(const semantics::concat_chain& chain) const
{
    diagnostic_consumer_transform diags([this](diagnostic_op d) {
        if (diagnoser_)
            diagnoser_->add_diagnostic(std::move(d));
    });
    for (const auto& e : chain)
        e->resolve(diags);
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

bool parser_impl::is_previous_attribute_consuming(bool top_level, const antlr4::Token* token)
{
    if (!token)
        return false;

    auto text = token->getText();
    if (text.size() != 1)
        return false;

    auto tmp = std::toupper((unsigned char)text.back());

    // this almost looks like a bug in the original assembler
    return tmp == 'O' && top_level || tmp == 'S' || tmp == 'I' || tmp == 'L' || tmp == 'T';
}

antlr4::misc::IntervalSet parser_impl::getExpectedTokens()
{
    if (proc_status->first.kind == processing::processing_kind::LOOKAHEAD)
        return {};
    else
        return antlr4::Parser::getExpectedTokens();
}

void parser_impl::add_diagnostic(
    diagnostic_severity severity, std::string code, std::string message, range diag_range) const
{
    add_diagnostic(diagnostic_op(severity, std::move(code), std::move(message), diag_range));
}

void parser_impl::add_diagnostic(diagnostic_op d) const
{
    if (diagnoser_)
        diagnoser_->add_diagnostic(std::move(d));
}

context::id_index parser_impl::add_id(std::string s) { return hlasm_ctx->ids().add(std::move(s)); }

parser_holder::~parser_holder() = default;

void parser_holder::prepare_parser(const std::string& text,
    context::hlasm_context* hlasm_ctx,
    diagnostic_op_consumer* diags,
    semantics::range_provider range_prov,
    range text_range,
    const processing::processing_status& proc_status,
    bool unlimited_line) const
{
    input->reset(text);

    lex->reset();
    lex->set_file_offset(text_range.start);
    lex->set_unlimited_line(unlimited_line);

    stream->reset();

    parser->reinitialize(hlasm_ctx, std::move(range_prov), proc_status, diags);

    parser->reset();

    parser->get_collector().prepare_for_next_statement();
}

} // namespace hlasm_plugin::parser_library::parsing
