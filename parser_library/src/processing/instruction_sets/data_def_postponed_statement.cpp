/*
 * Copyright (c) 2021 Broadcom.
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

#include "data_def_postponed_statement.h"

#include <limits>

#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

template<checking::data_instr_type instr_type>
data_def_postponed_statement<instr_type>::data_def_postponed_statement(rebuilt_statement stmt,
    context::processing_stack_t stmt_location_stack,
    std::vector<data_def_dependency<instr_type>> dependencies)
    : postponed_statement_impl(std::move(stmt), std::move(stmt_location_stack))
    , m_dependencies(std::move(dependencies))
{}

// Inherited via resolvable
template<checking::data_instr_type instr_type>
context::dependency_collector data_def_dependency<instr_type>::get_dependencies(
    context::dependency_solver& _solver) const
{
    data_def_dependency_solver solver(_solver, &m_loctr);
    context::dependency_collector deps;
    for (auto it = m_begin; it != m_end; ++it)
    {
        const auto& op = *it;
        if (op->type == semantics::operand_type::EMPTY)
            continue;
        deps.merge(op->access_data_def()->get_length_dependencies(solver));
    }
    return deps;
}

template<checking::data_instr_type instr_type>
int32_t data_def_dependency<instr_type>::get_operands_length(const semantics::operand_ptr* b,
    const semantics::operand_ptr* e,
    context::dependency_solver& _solver,
    diagnostic_op_consumer& diags,
    const context::address* loctr)
{
    data_def_dependency_solver solver(_solver, loctr);

    constexpr const auto round_up_bytes = [](uint64_t& v, uint64_t bytes) { v = checking::round_up(v, bytes * 8); };

    for (auto it = b; it != e; ++it)
    {
        const auto& op = *it;
        if (op->type == semantics::operand_type::EMPTY)
            continue;

        if (auto dd = op->access_data_def()->value.get();
            dd->length_type != expressions::data_definition::length_type::BIT)
        {
            // align to whole byte
            round_up_bytes(solver.operands_bit_length, 1);

            // enforce data def alignment
            round_up_bytes(solver.operands_bit_length, dd->get_alignment().boundary);
        }
        long long len = op->access_data_def()->evaluate_total_length(solver, diags);

        if (len < 0)
            return 0;

        solver.operands_bit_length += len;
    }

    // align to whole byte
    round_up_bytes(solver.operands_bit_length, 1);

    // returns the length in bytes
    uint64_t len = solver.operands_bit_length / 8;

    if (len > std::numeric_limits<int32_t>::max())
        return 0;
    else
        return (int32_t)len;
}

template<checking::data_instr_type instr_type>
context::symbol_value data_def_dependency<instr_type>::resolve(context::dependency_solver& solver) const
{
    diagnostic_consumer_transform drop_diags([](diagnostic_op) {});
    return get_operands_length(m_begin, m_end, solver, drop_diags, &m_loctr);
}

template class data_def_postponed_statement<checking::data_instr_type::DC>;
template class data_def_postponed_statement<checking::data_instr_type::DS>;
template class data_def_dependency<checking::data_instr_type::DC>;
template class data_def_dependency<checking::data_instr_type::DS>;

const context::symbol* data_def_dependency_solver::get_symbol(context::id_index name) const
{
    return base.get_symbol(name);
}

std::optional<context::address> data_def_dependency_solver::get_loctr() const
{
    if (loctr)
        return *loctr + (int)(operands_bit_length / 8);
    if (auto l = base.get_loctr(); l.has_value())
        return l.value() + (int)(operands_bit_length / 8);

    return std::nullopt;
}

context::id_index data_def_dependency_solver::get_literal_id(
    const std::shared_ptr<const expressions::data_definition>& dd)
{
    return base.get_literal_id(dd);
}

bool data_def_dependency_solver::using_active(context::id_index label, const context::section* sect)
{
    return base.using_active(label, sect);
}

} // namespace hlasm_plugin::parser_library::processing
