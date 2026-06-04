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

#include "occurrence_collector.h"

#include "context/hlasm_context.h"
#include "expressions/conditional_assembly/terms/ca_constant.h"
#include "expressions/conditional_assembly/terms/ca_expr_list.h"
#include "expressions/conditional_assembly/terms/ca_function.h"
#include "expressions/conditional_assembly/terms/ca_string.h"
#include "expressions/conditional_assembly/terms/ca_symbol.h"
#include "expressions/conditional_assembly/terms/ca_symbol_attribute.h"
#include "expressions/conditional_assembly/terms/ca_var_sym.h"
#include "expressions/mach_expression.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

occurrence_collector::occurrence_collector(lsp::occurrence_kind collector_kind,
    context::hlasm_context& hlasm_ctx,
    std::vector<lsp::symbol_occurrence>& storage,
    bool evaluated_model)
    : collector_kind(collector_kind)
    , hlasm_ctx(hlasm_ctx)
    , occurrences(storage)
    , evaluated_model(evaluated_model)
{}

void occurrence_collector::visit(const semantics::empty_operand&) {}

void occurrence_collector::visit(const semantics::model_operand& op) { get_occurrence(op.chain); }

void occurrence_collector::visit(const semantics::machine_operand& op)
{
    if (op.displacement)
        op.displacement->apply(*this);
    if (op.first_par)
        op.first_par->apply(*this);
    if (op.second_par)
        op.second_par->apply(*this);
}

void occurrence_collector::visit(const semantics::expr_assembler_operand& op)
{
    if (op.expression)
        op.expression->apply(*this);
}

void occurrence_collector::visit(const semantics::using_instr_assembler_operand& op)
{
    if (op.base)
        op.base->apply(*this);
    if (op.end)
        op.end->apply(*this);
}

void occurrence_collector::visit(const semantics::complex_assembler_operand&) {}

void occurrence_collector::visit(const semantics::string_assembler_operand&) {}

void occurrence_collector::visit(const semantics::text_assembler_operand& op)
{
    if (any(collector_kind & lsp::occurrence_kind::ORD) && !op.get_ord_like().empty())
        occurrences.emplace_back(lsp::occurrence_kind::ORD, op.get_ord_like(), op.operand_range, evaluated_model);
}

void occurrence_collector::visit(const semantics::data_def_operand& op) { op.value->apply(*this); }

void occurrence_collector::visit(const semantics::var_ca_operand& op) { get_occurrence(*op.variable_symbol); }

void occurrence_collector::visit(const semantics::expr_ca_operand& op)
{
    if (op.expression)
        op.expression->apply(*this);
}

void occurrence_collector::visit(const semantics::seq_ca_operand& op) { get_occurrence(op.sequence_symbol); }

void occurrence_collector::visit(const semantics::branch_ca_operand& op)
{
    op.expression->apply(*this);
    get_occurrence(op.sequence_symbol);
}

void occurrence_collector::visit(const semantics::macro_operand& op) { get_occurrence(op.chain); }

void occurrence_collector::get_occurrence(const semantics::variable_symbol& var)
{
    if (var.created)
        get_occurrence(var.access_created()->created_name);
    else if (any(collector_kind & lsp::occurrence_kind::VAR))
        occurrences.emplace_back(
            lsp::occurrence_kind::VAR, var.access_basic()->name, var.symbol_range, evaluated_model);
}

void occurrence_collector::get_occurrence(const semantics::seq_sym& seq)
{
    if (any(collector_kind & lsp::occurrence_kind::SEQ))
        occurrences.emplace_back(lsp::occurrence_kind::SEQ, seq.name, seq.symbol_range, evaluated_model);
}

void occurrence_collector::get_occurrence(context::id_index ord, const range& ord_range)
{
    if (any(collector_kind & lsp::occurrence_kind::ORD))
        occurrences.emplace_back(lsp::occurrence_kind::ORD, ord, ord_range, evaluated_model);
}

void occurrence_collector::get_occurrence(const semantics::concat_chain& chain)
{
    for (const auto& point : chain)
    {
        if (const auto* str = std::get_if<semantics::char_str_conc>(&point.value))
        {
            if (any(collector_kind & lsp::occurrence_kind::ORD))
            {
                auto [valid, name] = hlasm_ctx.try_get_symbol_name(str->value);
                if (valid)
                    occurrences.emplace_back(lsp::occurrence_kind::ORD, name, str->conc_range, evaluated_model);
            }
        }
        else if (const auto* var = std::get_if<semantics::var_sym_conc>(&point.value))
        {
            get_occurrence(*var->symbol);
        }
        else if (const auto* sub = std::get_if<semantics::sublist_conc>(&point.value))
        {
            for (const auto& ch : sub->list)
                get_occurrence(ch);
        }
    }
}
void occurrence_collector::get_occurrence(const semantics::literal_si& lit)
{
    if (any(collector_kind & lsp::occurrence_kind::ORD))
    {
        lit->get_dd().apply(*this);
    }
}

void occurrence_collector::visit(const expressions::mach_expr_constant&) {}

void occurrence_collector::visit(const expressions::mach_expr_data_attr& expr)
{
    get_occurrence(expr.value, expr.symbol_range);
    if (!expr.qualifier.empty())
        get_occurrence(expr.qualifier, expr.symbol_range);
}

void occurrence_collector::visit(const expressions::mach_expr_data_attr_literal& expr) { visit(*expr.lit); }

void occurrence_collector::visit(const expressions::mach_expr_symbol& expr)
{
    get_occurrence(expr.value, expr.get_range());
    if (!expr.qualifier.empty())
        get_occurrence(expr.qualifier, expr.get_range());
}

void occurrence_collector::visit(const expressions::mach_expr_location_counter&) {}

void occurrence_collector::visit(const expressions::mach_expr_literal& lit) { lit.get_data_definition().apply(*this); }

void occurrence_collector::visit(const expressions::ca_constant&) {}

void occurrence_collector::visit(const expressions::ca_expr_list& expr)
{
    for (auto& e : expr.expression_list())
        e->apply(*this);
}

void occurrence_collector::visit(const expressions::ca_function& expr)
{
    if (expr.duplication_factor)
        expr.duplication_factor->apply(*this);
    for (const auto& e : expr.parameters)
        e->apply(*this);
}

void occurrence_collector::visit(const expressions::ca_string& expr)
{
    if (expr.duplication_factor)
        expr.duplication_factor->apply(*this);
    get_occurrence(expr.value);
    if (expr.substring.start)
        expr.substring.start->apply(*this);
    if (expr.substring.count)
        expr.substring.count->apply(*this);
}

void occurrence_collector::visit(const expressions::ca_symbol& expr) { get_occurrence(expr.symbol, expr.expr_range); }

void occurrence_collector::visit(const expressions::ca_symbol_attribute& expr)
{
    if (std::holds_alternative<context::id_index>(expr.symbol))
        get_occurrence(std::get<context::id_index>(expr.symbol), expr.symbol_range);
    else if (std::holds_alternative<semantics::vs_ptr>(expr.symbol))
        get_occurrence(*std::get<semantics::vs_ptr>(expr.symbol));
    else if (std::holds_alternative<semantics::literal_si>(expr.symbol))
        get_occurrence(std::get<semantics::literal_si>(expr.symbol));
}

void occurrence_collector::visit(const expressions::ca_var_sym& expr) { get_occurrence(*expr.symbol); }

} // namespace hlasm_plugin::parser_library::processing
