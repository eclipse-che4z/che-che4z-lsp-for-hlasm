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

#ifndef HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H
#define HLASMPLUGIN_PARSERLIBRARY_PROCESSING_DATA_DEF_POSTPONED_STATEMENT_H

#include <variant>

#include "checking/data_definition/data_definition_operand.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::processing {

template<checking::data_instr_type instr_type>
struct data_def_postponed_statement : public postponed_statement_impl, public context::resolvable
{
    data_def_postponed_statement(rebuilt_statement stmt, context::processing_stack_t stmt_location_stack)
        : postponed_statement_impl(std::move(stmt), std::move(stmt_location_stack))
    {}


    // Inherited via resolvable
    context::dependency_collector get_dependencies(context::dependency_solver& solver) const override
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

    static int32_t get_operands_length(const semantics::operand_list& operands, context::dependency_solver& _solver)
    {
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
                const range& r) override
            {
                return base.get_literal_id(text, dd, r);
            }
        } solver(_solver);

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

        if (len > INT32_MAX)
            return 0;
        else
            return (int32_t)len;
    }

    context::symbol_value resolve(context::dependency_solver& solver) const override
    {
        return get_operands_length(operands_ref().value, solver);
    }
};

} // namespace hlasm_plugin::parser_library::processing
#endif