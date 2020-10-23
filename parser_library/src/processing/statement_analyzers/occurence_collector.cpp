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

#include "processing/context_manager.h"

namespace hlasm_plugin::parser_library::processing {

occurence_collector::occurence_collector(lsp::occurence_kind collector_kind, lsp::occurence_storage& storage)
    : collector_kind(collector_kind)
    , occurences(storage)
{}

void occurence_collector::visit(const semantics::empty_operand&) {}

void occurence_collector::visit(const semantics::model_operand& op) { get_occurence(op.chain); }

void occurence_collector::visit(const semantics::expr_machine_operand& op) { op.expression->apply(*this); }

void occurence_collector::visit(const semantics::address_machine_operand& op)
{
    op.displacement->apply(*this);
    op.first_par->apply(*this);
    op.second_par->apply(*this);
}

void occurence_collector::visit(const semantics::expr_assembler_operand& op) { op.expression->apply(*this); }

void occurence_collector::visit(const semantics::using_instr_assembler_operand& op)
{
    op.base->apply(*this);
    op.end->apply(*this);
}

void occurence_collector::visit(const semantics::complex_assembler_operand&) {}

void occurence_collector::visit(const semantics::string_assembler_operand&) {}

void occurence_collector::visit(const semantics::data_def_operand& op)
{
    if (op.value->dupl_factor)
        op.value->dupl_factor->apply(*this);
    if (op.value->program_type)
        op.value->program_type->apply(*this);
    if (op.value->length)
        op.value->length->apply(*this);
    if (op.value->scale)
        op.value->scale->apply(*this);
    if (op.value->exponent)
        op.value->exponent->apply(*this);

    if (op.value->nominal_value && op.value->nominal_value->access_exprs())
    {
        for (const auto& val : op.value->nominal_value->access_exprs()->exprs)
        {
            if (std::holds_alternative<expressions::mach_expr_ptr>(val))
                std::get<expressions::mach_expr_ptr>(val)->apply(*this);
            else
            {
                const auto& addr = std::get<expressions::address_nominal>(val);
                addr.base->apply(*this);
                addr.displacement->apply(*this);
            }
        }
    }
}

void occurence_collector::visit(const semantics::var_ca_operand& op) { get_occurence(*op.variable_symbol); }

void occurence_collector::visit(const semantics::expr_ca_operand& op) { op.expression->apply(*this); }

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
            occurences.push_back(
                lsp::symbol_occurence { lsp::occurence_kind::VAR, var.access_basic()->name, var.symbol_range });
    }
}

void occurence_collector::get_occurence(const semantics::seq_sym& seq)
{
    if (collector_kind == lsp::occurence_kind::SEQ)
        occurences.push_back(lsp::symbol_occurence { lsp::occurence_kind::ORD, seq.name, seq.symbol_range });
}

void occurence_collector::get_occurence(context::id_index ord, const range& ord_range)
{
    if (collector_kind == lsp::occurence_kind::ORD)
        occurences.push_back(lsp::symbol_occurence { lsp::occurence_kind::ORD, ord, ord_range });
}

void occurence_collector::get_occurence(const semantics::concat_chain& chain)
{
    for (const auto& point : chain)
    {
        switch (point->type)
        {
            case semantics::concat_type::STR:
                /* TODO add range to str_conc struct
                if (collector_kind == lsp::occurence_kind::ORD)
                {
                    auto [valid, name] =
                processing::context_manager(hlasm_ctx).try_get_symbol_name(point->access_str()->value); if (valid)
                        occurences.push_back(
                            lsp::symbol_occurence { lsp::occurence_kind::ORD, name, point-> });
                }*/
                break;
            case semantics::concat_type::VAR:
                if (collector_kind == lsp::occurence_kind::VAR)
                    get_occurence(*point->access_var()->access_var()->symbol);
                break;
            case semantics::concat_type::SUB:
                for (const auto& ch : point->access_sub()->list)
                    get_occurence(ch);
                break;
            default:
                break;
        }
    }
}

void occurence_collector::visit(const expressions::mach_expr_constant&) {}

void occurence_collector::visit(const expressions::mach_expr_data_attr& expr)
{
    get_occurence(expr.value, expr.get_range());
}

void occurence_collector::visit(const expressions::mach_expr_symbol& expr)
{
    get_occurence(expr.value, expr.get_range());
}

void occurence_collector::visit(const expressions::mach_expr_location_counter&) {}

void occurence_collector::visit(const expressions::mach_expr_self_def&) {}

void occurence_collector::visit(const expressions::mach_expr_default&) {}

void occurence_collector::visit(const expressions::ca_constant&) {}

void occurence_collector::visit(const expressions::ca_expr_list&) {}

void occurence_collector::visit(const expressions::ca_function& expr)
{
    if (expr.duplication_factor)
        expr.duplication_factor->apply(*this);
    for (const auto& e : expr.parameters)
        e->apply(*this);
}

void occurence_collector::visit(const expressions::ca_string&) {}

void occurence_collector::visit(const expressions::ca_symbol& expr) { get_occurence(expr.symbol, expr.expr_range); }

void occurence_collector::visit(const expressions::ca_symbol_attribute& expr)
{
    if (std::holds_alternative<context::id_index>(expr.symbol))
        get_occurence(std::get<context::id_index>(expr.symbol), expr.expr_range);
    else
        get_occurence(*std::get<semantics::vs_ptr>(expr.symbol));
}

void occurence_collector::visit(const expressions::ca_var_sym& expr) { get_occurence(*expr.symbol); }

} // namespace hlasm_plugin::parser_library::processing
