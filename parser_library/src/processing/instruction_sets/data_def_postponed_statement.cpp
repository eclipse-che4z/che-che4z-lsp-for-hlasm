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
data_def_postponed_statement<instr_type>::data_def_postponed_statement(
    rebuilt_statement stmt, context::processing_stack_t stmt_location_stack)
    : postponed_statement_impl(std::move(stmt), std::move(stmt_location_stack))
{}

// Inherited via resolvable
template<checking::data_instr_type instr_type>
context::dependency_collector data_def_postponed_statement<instr_type>::get_dependencies(
    context::dependency_solver& solver) const
{
    context::dependency_collector conjunction;
    for (const auto& op : operands_ref().value)
    {
        if (op->type == semantics::operand_type::EMPTY)
            continue;
        conjunction = conjunction + op->access_data_def()->get_length_dependencies(solver);
    }
    return conjunction;
}

namespace {
struct data_def_dependency_solver final : public context::dependency_solver
{
    explicit data_def_dependency_solver(context::dependency_solver& base)
        : base(base)
    {}
    context::dependency_solver& base;
    uint64_t operands_bit_length = 0;

    const context::symbol* get_symbol(context::id_index name) const override { return base.get_symbol(name); }
    std::optional<context::address> get_loctr() const override
    {
        if (auto loctr = base.get_loctr(); loctr.has_value())
            return loctr.value() + (int)(operands_bit_length / 8);

        return std::nullopt;
    }
    context::id_index get_literal_id(const std::string& text,
        const std::shared_ptr<const expressions::data_definition>& dd,
        const range& r,
        bool align_on_halfword) override
    {
        return base.get_literal_id(text, dd, r, align_on_halfword);
    }
};
} // namespace

template<checking::data_instr_type instr_type>
int32_t data_def_postponed_statement<instr_type>::get_operands_length(
    const semantics::operand_list& operands, context::dependency_solver& _solver)
{
    data_def_dependency_solver solver(_solver);

    constexpr const auto round_up_bytes = [](uint64_t& v, uint64_t bytes) { v = checking::round_up(v, bytes * 8); };

    for (const auto& op : operands)
    {
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
        const auto o = op->access_data_def()->get_operand_value(solver);
        const auto* dd_op = dynamic_cast<checking::data_definition_operand*>(o.get());


        if (!dd_op->check<instr_type>(diagnostic_collector()))
            return 0;

        solver.operands_bit_length += dd_op->get_length();
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
context::symbol_value data_def_postponed_statement<instr_type>::resolve(context::dependency_solver& solver) const
{
    return get_operands_length(operands_ref().value, solver);
}

template struct data_def_postponed_statement<checking::data_instr_type::DC>;
template struct data_def_postponed_statement<checking::data_instr_type::DS>;

} // namespace hlasm_plugin::parser_library::processing
