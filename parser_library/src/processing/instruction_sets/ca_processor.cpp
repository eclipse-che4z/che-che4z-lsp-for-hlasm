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

#include "ca_processor.h"

#include "semantics/operand_impls.h"
#include "semantics/range_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace processing;
using namespace workspaces;

ca_processor::ca_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    processing_state_listener& listener)
    : instruction_processor(ctx, branch_provider, lib_provider)
    , table_(create_table(*ctx.hlasm_ctx))
    , listener_(listener)
{}

void ca_processor::process(context::shared_stmt_ptr stmt)
{
    auto res = stmt->access_resolved();
    auto& func = table_.at(res->opcode_ref().value);
    func(*res);
}

ca_processor::process_table_t ca_processor::create_table(context::hlasm_context& h_ctx)
{
    process_table_t table;
    table.emplace(
        h_ctx.ids().add("SETA"), std::bind(&ca_processor::process_SET<context::A_t>, this, std::placeholders::_1));
    table.emplace(
        h_ctx.ids().add("SETB"), std::bind(&ca_processor::process_SET<context::B_t>, this, std::placeholders::_1));
    table.emplace(
        h_ctx.ids().add("SETC"), std::bind(&ca_processor::process_SET<context::C_t>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("LCLA"),
        std::bind(&ca_processor::process_GBL_LCL<context::A_t, false>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("LCLB"),
        std::bind(&ca_processor::process_GBL_LCL<context::B_t, false>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("LCLC"),
        std::bind(&ca_processor::process_GBL_LCL<context::C_t, false>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("GBLA"),
        std::bind(&ca_processor::process_GBL_LCL<context::A_t, true>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("GBLB"),
        std::bind(&ca_processor::process_GBL_LCL<context::B_t, true>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("GBLC"),
        std::bind(&ca_processor::process_GBL_LCL<context::C_t, true>, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("ANOP"), std::bind(&ca_processor::process_ANOP, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("ACTR"), std::bind(&ca_processor::process_ACTR, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("AGO"), std::bind(&ca_processor::process_AGO, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("AIF"), std::bind(&ca_processor::process_AIF, this, std::placeholders::_1));
    table.emplace(context::id_storage::empty_id, std::bind(&ca_processor::process_empty, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("MACRO"), std::bind(&ca_processor::process_MACRO, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("MEND"), std::bind(&ca_processor::process_MEND, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("MEXIT"), std::bind(&ca_processor::process_MEXIT, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("AREAD"), std::bind(&ca_processor::process_AREAD, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("ASPACE"), std::bind(&ca_processor::process_ASPACE, this, std::placeholders::_1));
    table.emplace(h_ctx.ids().add("AEJECT"), std::bind(&ca_processor::process_AEJECT, this, std::placeholders::_1));

    return table;
}

void ca_processor::register_seq_sym(const semantics::complete_statement& stmt)
{
    if (stmt.label_ref().type == semantics::label_si_type::SEQ)
    {
        auto symbol = std::get<semantics::seq_sym>(stmt.label_ref().value);
        branch_provider.register_sequence_symbol(symbol.name, symbol.symbol_range);
    }
    else if (stmt.label_ref().type != semantics::label_si_type::EMPTY)
    {
        add_diagnostic(diagnostic_op::warning_W010("Name field", stmt.label_ref().field_range));
    }
}

bool ca_processor::test_symbol_for_assignment(const semantics::variable_symbol* symbol,
    context::SET_t_enum type,
    int& idx,
    context::set_symbol_base*& set_symbol,
    context::id_index& name)
{
    set_symbol = nullptr;
    idx = -1;

    auto [tmp_name, subscript] = symbol->evaluate_symbol(eval_ctx);
    name = tmp_name;

    auto var_symbol = hlasm_ctx.get_var_sym(name);

    if (var_symbol && var_symbol->access_macro_param_base())
    {
        add_diagnostic(diagnostic_op::error_E030("symbolic parameter", symbol->symbol_range));
        return false;
    }

    if (subscript.size() > 1)
    {
        add_diagnostic(diagnostic_op::error_E020("variable symbol subscript", symbol->symbol_range));
        return false;
    }
    else if (subscript.size() == 1)
    {
        idx = symbol->subscript.front()->evaluate<context::A_t>(eval_ctx);

        if (subscript.front() < 1)
        {
            add_diagnostic(diagnostic_op::error_E012("subscript value has to be 1 or more", symbol->symbol_range));
            return false;
        }
    }

    if (!var_symbol)
        return true;

    auto set_sym = var_symbol->access_set_symbol_base();
    assert(set_sym);
    if (set_sym->type != type)
    {
        add_diagnostic(diagnostic_op::error_E013("wrong type of variable symbol", symbol->symbol_range));
        return false;
    }

    if ((set_sym->is_scalar && symbol->subscript.size() == 1) || (!set_sym->is_scalar && symbol->subscript.size() == 0))
    {
        add_diagnostic(diagnostic_op::error_E013("subscript error", symbol->symbol_range));
        return false;
    }

    set_symbol = set_sym;
    return true;
}

bool ca_processor::prepare_SET_symbol(const semantics::complete_statement& stmt,
    context::SET_t_enum type,
    int& idx,
    context::set_symbol_base*& set_symbol,
    context::id_index& name)
{
    if (stmt.label_ref().type != semantics::label_si_type::VAR)
    {
        add_diagnostic(diagnostic_op::error_E010("label", stmt.label_ref().field_range));
        return false;
    }

    auto symbol = std::get<semantics::vs_ptr>(stmt.label_ref().value).get();

    auto ok = test_symbol_for_assignment(symbol, type, idx, set_symbol, name);

    return ok;
}

bool ca_processor::prepare_SET_operands(
    const semantics::complete_statement& stmt, std::vector<expressions::ca_expression*>& expr_values)
{
    bool has_operand = false;
    for (auto& op : stmt.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
            continue;

        has_operand = true;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind != semantics::ca_kind::VAR && ca_op->kind != semantics::ca_kind::EXPR)
        {
            add_diagnostic(diagnostic_op::error_E012("SET instruction", ca_op->operand_range));
            return false;
        }

        expr_values.push_back(ca_op->access_expr()->expression.get());
    }

    if (!has_operand)
    {
        add_diagnostic(diagnostic_op::error_E022("SET instruction", stmt.instruction_ref().field_range));
        return false;
    }
    return true;
}

bool ca_processor::prepare_GBL_LCL(
    const semantics::complete_statement& stmt, std::vector<context::id_index>& ids, std::vector<bool>& scalar_info)
{
    bool has_operand = false;
    for (auto& op : stmt.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
            continue;

        has_operand = true;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind == semantics::ca_kind::VAR)
        {
            auto [id, subscript] = ca_op->access_var()->variable_symbol->evaluate_symbol(eval_ctx);

            if (id == context::id_storage::empty_id)
                continue;

            if (auto var_sym = hlasm_ctx.get_var_sym(id))
            {
                if (var_sym->access_set_symbol_base())
                    add_diagnostic(diagnostic_op::error_E051(*id, ca_op->operand_range));
                else if (var_sym->access_macro_param_base())
                    add_diagnostic(diagnostic_op::error_E052(*id, ca_op->operand_range));
                else
                    assert(false);
                continue;
            }
            if (std::find(ids.begin(), ids.end(), id) != ids.end())
            {
                add_diagnostic(diagnostic_op::error_E051(*id, ca_op->operand_range));
            }
            else
            {
                ids.push_back(id);
                scalar_info.push_back(subscript.empty());
            }
        }
        else
        {
            add_diagnostic(diagnostic_op::error_E010("operand", ca_op->operand_range));
            return false;
        }
    }

    if (!has_operand)
    {
        add_diagnostic(diagnostic_op::error_E022("variable symbol definition", stmt.instruction_ref().field_range));
        return false;
    }

    return true;
}

void ca_processor::process_ANOP(const semantics::complete_statement& stmt)
{
    assert(stmt.operands_ref().value.empty());
    register_seq_sym(stmt);
}

bool ca_processor::prepare_ACTR(const semantics::complete_statement& stmt, context::A_t& ctr)
{
    if (stmt.operands_ref().value.size() != 1)
    {
        add_diagnostic(diagnostic_op::error_E020("operand", stmt.instruction_ref().field_range));
        return false;
    }

    const auto* ca_op = stmt.operands_ref().value[0]->access_ca();
    assert(ca_op);

    if (ca_op->kind == semantics::ca_kind::EXPR || ca_op->kind == semantics::ca_kind::VAR)
    {
        ctr = ca_op->access_expr()->expression->evaluate<context::A_t>(eval_ctx);
        return true;
    }
    else
    {
        add_diagnostic(diagnostic_op::error_E010("operand", ca_op->operand_range));
        return false;
    }
}

void ca_processor::process_ACTR(const semantics::complete_statement& stmt)
{
    register_seq_sym(stmt);

    context::A_t ctr;
    bool ok = prepare_ACTR(stmt, ctr);

    if (ok)
        hlasm_ctx.set_branch_counter(ctr);
}

bool ca_processor::prepare_AGO(const semantics::complete_statement& stmt,
    context::A_t& branch,
    std::vector<std::pair<context::id_index, range>>& targets)
{
    if (stmt.operands_ref().value.empty())
    {
        add_diagnostic(diagnostic_op::error_E022("AGO", stmt.instruction_ref().field_range));
        return false;
    }

    for (const auto& op : stmt.operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
        {
            add_diagnostic(diagnostic_op::error_E010("operand", op->operand_range));
            return false;
        }
    }

    auto ca_op = stmt.operands_ref().value[0]->access_ca();
    assert(ca_op);

    if (ca_op->kind == semantics::ca_kind::SEQ)
    {
        if (stmt.operands_ref().value.size() != 1)
        {
            add_diagnostic(diagnostic_op::error_E010("operand", ca_op->operand_range));
            return false;
        }

        auto& symbol = ca_op->access_seq()->sequence_symbol;
        targets.emplace_back(symbol.name, symbol.symbol_range);
        branch = 1;
        return true;
    }

    if (ca_op->kind == semantics::ca_kind::BRANCH)
    {
        auto br_op = ca_op->access_branch();
        branch = br_op->expression->evaluate<context::A_t>(eval_ctx);
        targets.emplace_back(br_op->sequence_symbol.name, br_op->sequence_symbol.symbol_range);

        for (size_t i = 1; i < stmt.operands_ref().value.size(); ++i)
        {
            auto tmp = stmt.operands_ref().value[i]->access_ca();
            assert(tmp);

            if (tmp->kind == semantics::ca_kind::SEQ)
            {
                auto& symbol = tmp->access_seq()->sequence_symbol;
                targets.emplace_back(symbol.name, symbol.symbol_range);
            }
            else
            {
                add_diagnostic(diagnostic_op::error_E010("operand", tmp->operand_range));
                return false;
            }
        }
        return true;
    }
    return false;
}

void ca_processor::process_AGO(const semantics::complete_statement& stmt)
{
    register_seq_sym(stmt);

    context::A_t branch;
    std::vector<std::pair<context::id_index, range>> targets;
    bool ok = prepare_AGO(stmt, branch, targets);
    if (!ok)
        return;

    if (branch > 0 && branch <= (context::A_t)targets.size())
        branch_provider.jump_in_statements(targets[branch - 1].first, targets[branch - 1].second);
}

bool ca_processor::prepare_AIF(
    const semantics::complete_statement& stmt, context::B_t& condition, context::id_index& target, range& target_range)
{
    condition = false;

    if (stmt.operands_ref().value.empty())
    {
        add_diagnostic(diagnostic_op::error_E022("AIF", stmt.instruction_ref().field_range));
        return false;
    }

    bool has_operand = false;
    for (auto it = stmt.operands_ref().value.begin(); it != stmt.operands_ref().value.end(); ++it)
    {
        const auto& op = *it;

        if (op->type == semantics::operand_type::EMPTY)
        {
            if (it == stmt.operands_ref().value.end() - 1)
                continue;

            add_diagnostic(diagnostic_op::error_E010("operand", op->operand_range));
            return false;
        }
        has_operand = true;

        auto ca_op = op->access_ca();
        assert(ca_op);

        if (ca_op->kind == semantics::ca_kind::BRANCH)
        {
            if (!condition)
            {
                auto br = ca_op->access_branch();
                condition = br->expression->evaluate<context::B_t>(eval_ctx);

                target = br->sequence_symbol.name;
                target_range = br->sequence_symbol.symbol_range;
            }
        }
        else
        {
            add_diagnostic(diagnostic_op::error_E010("operand", ca_op->operand_range));
            return false;
        }
    }

    if (!has_operand)
    {
        add_diagnostic(diagnostic_op::error_E022("variable symbol definition", stmt.instruction_ref().field_range));
        return false;
    }

    return true;
}

void ca_processor::process_AIF(const semantics::complete_statement& stmt)
{
    register_seq_sym(stmt);

    context::B_t condition;
    context::id_index target;
    range target_range;
    bool ok = prepare_AIF(stmt, condition, target, target_range);

    if (!ok)
        return;

    if (condition)
        branch_provider.jump_in_statements(target, target_range);
}

void ca_processor::process_MACRO(const semantics::complete_statement& stmt)
{
    register_seq_sym(stmt);
    listener_.start_macro_definition({});
}

void ca_processor::process_MEXIT(const semantics::complete_statement& stmt)
{
    if (!hlasm_ctx.is_in_macro())
        add_diagnostic(diagnostic_op::error_E054(stmt.stmt_range_ref()));
    else
        hlasm_ctx.leave_macro();
}

void ca_processor::process_MEND(const semantics::complete_statement& stmt)
{
    if (!hlasm_ctx.is_in_macro())
        add_diagnostic(diagnostic_op::error_E054(stmt.stmt_range_ref()));
}

void ca_processor::process_AEJECT(const semantics::complete_statement&)
{
    // TODO
}

void ca_processor::process_ASPACE(const semantics::complete_statement& stmt)
{
    // TODO
    (void)stmt;
}

void ca_processor::process_AREAD(const semantics::complete_statement& stmt)
{
    if (stmt.label_ref().type != semantics::label_si_type::VAR)
    {
        add_diagnostic(diagnostic_op::error_E010("label", stmt.label_ref().field_range));
        return;
    }

    int index;
    context::id_index name;
    context::set_symbol_base* set_symbol;
    bool ok = prepare_SET_symbol(stmt, context::SET_t_enum::C_TYPE, index, set_symbol, name);

    if (!ok)
        return;

    if (!set_symbol)
        set_symbol = hlasm_ctx.create_local_variable<context::C_t>(name, index == -1).get();

    // TODO read proper input line
    auto empty_line = "                                                                                ";

    set_symbol->access_set_symbol<context::C_t>()->set_value(empty_line, index - 1);
}

void ca_processor::process_empty(const semantics::complete_statement&) {}
