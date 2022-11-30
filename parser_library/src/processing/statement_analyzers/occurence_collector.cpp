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

#include "occurence_collector.h"

#include "context/hlasm_context.h"
#include "expressions/mach_expression.h"

namespace hlasm_plugin::parser_library::processing {

occurence_collector::occurence_collector(lsp::occurence_kind collector_kind,
    context::hlasm_context& hlasm_ctx,
    lsp::occurence_storage& storage,
    bool evaluated_model)
    : collector_kind(collector_kind)
    , hlasm_ctx(hlasm_ctx)
    , occurences(storage)
    , evaluated_model(evaluated_model)
{}

void occurence_collector::visit(const semantics::empty_operand&) {}

void occurence_collector::visit(const semantics::model_operand& op) { get_occurence(op.chain); }

void occurence_collector::visit(const semantics::expr_machine_operand& op)
{
    if (op.expression)
        op.expression->apply(*this);
}

void occurence_collector::visit(const semantics::address_machine_operand& op)
{
    if (op.displacement)
        op.displacement->apply(*this);
    if (op.first_par)
        op.first_par->apply(*this);
    if (op.second_par)
        op.second_par->apply(*this);
}

void occurence_collector::visit(const semantics::expr_assembler_operand& op)
{
    if (op.expression)
        op.expression->apply(*this);
}

void occurence_collector::visit(const semantics::using_instr_assembler_operand& op)
{
    if (op.base)
        op.base->apply(*this);
    if (op.end)
        op.end->apply(*this);
}

void occurence_collector::visit(const semantics::complex_assembler_operand&) {}

void occurence_collector::visit(const semantics::string_assembler_operand&) {}

void occurence_collector::visit(const semantics::data_def_operand& op) { op.value->apply(*this); }

void occurence_collector::visit(const semantics::var_ca_operand& op) { get_occurence(*op.variable_symbol); }

void occurence_collector::visit(const semantics::expr_ca_operand& op)
{
    if (op.expression)
        op.expression->apply(*this);
}

void occurence_collector::visit(const semantics::seq_ca_operand& op) { get_occurence(op.sequence_symbol); }

void occurence_collector::visit(const semantics::branch_ca_operand& op)
{
    op.expression->apply(*this);
    get_occurence(op.sequence_symbol);
}

void occurence_collector::visit(const semantics::macro_operand_chain& op) { get_occurence(op.chain); }

void occurence_collector::visit(const semantics::macro_operand_string&) {}

void occurence_collector::get_occurence(const semantics::variable_symbol& var)
{
    if (collector_kind == lsp::occurence_kind::VAR)
    {
        if (var.created)
            get_occurence(var.access_created()->created_name);
        else
            occurences.emplace_back(
                lsp::occurence_kind::VAR, var.access_basic()->name, var.symbol_range, evaluated_model);
    }
}

void occurence_collector::get_occurence(const semantics::seq_sym& seq)
{
    if (collector_kind == lsp::occurence_kind::SEQ)
        occurences.emplace_back(lsp::occurence_kind::SEQ, seq.name, seq.symbol_range, evaluated_model);
}

void occurence_collector::get_occurence(context::id_index ord, const range& ord_range)
{
    if (collector_kind == lsp::occurence_kind::ORD)
        occurences.emplace_back(lsp::occurence_kind::ORD, ord, ord_range, evaluated_model);
}

void occurence_collector::get_occurence(const semantics::concat_chain& chain)
{
    for (const auto& point : chain)
    {
        if (const auto* str = std::get_if<semantics::char_str_conc>(&point.value))
        {
            if (collector_kind == lsp::occurence_kind::ORD)
            {
                auto [valid, name] = hlasm_ctx.try_get_symbol_name(str->value);
                if (valid)
                    occurences.emplace_back(lsp::occurence_kind::ORD, name, str->conc_range, evaluated_model);
            }
        }
        else if (const auto* var = std::get_if<semantics::var_sym_conc>(&point.value))
        {
            if (collector_kind == lsp::occurence_kind::VAR)
                get_occurence(*var->symbol);
        }
        else if (const auto* sub = std::get_if<semantics::sublist_conc>(&point.value))
        {
            for (const auto& ch : sub->list)
                get_occurence(ch);
        }
    }
}
void occurence_collector::get_occurence(const semantics::literal_si& lit)
{
    if (collector_kind == lsp::occurence_kind::ORD)
    {
        lit->get_dd().apply(*this);
    }
}

void occurence_collector::visit(const expressions::mach_expr_constant&) {}

void occurence_collector::visit(const expressions::mach_expr_data_attr& expr)
{
    get_occurence(expr.value, expr.symbol_range);
    if (!expr.qualifier.empty())
        get_occurence(expr.qualifier, expr.symbol_range);
}

void occurence_collector::visit(const expressions::mach_expr_data_attr_literal& expr) { visit(*expr.lit); }

void occurence_collector::visit(const expressions::mach_expr_symbol& expr)
{
    get_occurence(expr.value, expr.get_range());
    if (!expr.qualifier.empty())
        get_occurence(expr.qualifier, expr.get_range());
}

void occurence_collector::visit(const expressions::mach_expr_location_counter&) {}

void occurence_collector::visit(const expressions::mach_expr_default&) {}

void occurence_collector::visit(const expressions::mach_expr_literal& lit) { lit.get_data_definition().apply(*this); }

void occurence_collector::visit(const expressions::ca_constant&) {}

void occurence_collector::visit(const expressions::ca_expr_list& expr)
{
    for (auto& e : expr.expression_list())
        e->apply(*this);
}

void occurence_collector::visit(const expressions::ca_function& expr)
{
    if (expr.duplication_factor)
        expr.duplication_factor->apply(*this);
    for (const auto& e : expr.parameters)
        e->apply(*this);
}

void occurence_collector::visit(const expressions::ca_string& expr)
{
    if (expr.duplication_factor)
        expr.duplication_factor->apply(*this);
    get_occurence(expr.value);
    if (expr.substring.start)
        expr.substring.start->apply(*this);
    if (expr.substring.count)
        expr.substring.count->apply(*this);
}

void occurence_collector::visit(const expressions::ca_symbol& expr) { get_occurence(expr.symbol, expr.expr_range); }

void occurence_collector::visit(const expressions::ca_symbol_attribute& expr)
{
    if (std::holds_alternative<context::id_index>(expr.symbol))
        get_occurence(std::get<context::id_index>(expr.symbol), expr.symbol_range);
    else if (std::holds_alternative<semantics::vs_ptr>(expr.symbol))
        get_occurence(*std::get<semantics::vs_ptr>(expr.symbol));
    else if (std::holds_alternative<semantics::literal_si>(expr.symbol))
        get_occurence(std::get<semantics::literal_si>(expr.symbol));
}

void occurence_collector::visit(const expressions::ca_var_sym& expr) { get_occurence(*expr.symbol); }

} // namespace hlasm_plugin::parser_library::processing
