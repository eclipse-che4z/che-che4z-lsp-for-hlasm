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

// This file implements checking of data definition
// asm instructions from the assembler checker:
// DC, DS, DXD

#include "../data_check.h"
#include "checking/asm_instr_check.h"
#include "checking/diagnostic_collector.h"
#include "checking/using_label_checker.h"
#include "data_definition_operand.h"
#include "instructions/instruction.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::checking {

namespace {
std::pair<const data_def_type*, bool> check_type_and_extension(
    const expressions::data_definition& dd, const diagnostic_collector& add_diagnostic)
{
    auto found = data_def_type::types_and_extensions.find({ dd.type, dd.extension });

    if (found != data_def_type::types_and_extensions.end())
        return { found->second.get(), true };

    if (dd.extension)
    {
        found = data_def_type::types_and_extensions.find({ dd.type, '\0' });
        if (found != data_def_type::types_and_extensions.end())
        {
            add_diagnostic(diagnostic_op::error_D013(dd.extension_range, std::string_view(&dd.type, 1)));
            return { found->second.get(), false };
        }
    }

    add_diagnostic(diagnostic_op::error_D012(dd.type_range));
    return { nullptr, false };
}

data_instr_type translate(instructions::data_def_instruction t) noexcept
{
    switch (t)
    {
        case instructions::data_def_instruction::DC_TYPE:
            return data_instr_type::DC;
        case instructions::data_def_instruction::DS_TYPE:
            return data_instr_type::DS;
        default:
            assert(false);
    }
}
} // namespace

void check_data_instruction_operands(const instructions::assembler_instruction& ai,
    std::span<const std::unique_ptr<semantics::operand>> ops,
    const range& stmt_range,
    context::dependency_solver& dep_solver,
    diagnostic_collector& add_diagnostic)
{
    if (ops.empty())
    {
        add_diagnostic(diagnostic_op::error_A010_minimum(ai.name(), 1, stmt_range));
        return;
    }

    const auto subtype = translate(ai.data_def_type());

    diagnostic_consumer_transform diags([&add_diagnostic](diagnostic_op d) { add_diagnostic(std::move(d)); });
    checking::using_label_checker lc(dep_solver, diags);
    std::vector<context::id_index> missing_symbols;

    auto operands_bit_length = 0ULL;
    for (const auto& operand : ops)
    {
        const auto* op = operand->access_data_def();
        if (!op)
        {
            add_diagnostic(diagnostic_op::error_A004_data_def_expected(operand->operand_range));
            continue;
        }

        assert(ai.has_ord_symbols());
        assert(!ai.postpone_dependencies());

        if (const auto deps = op->value->get_dependencies(dep_solver); deps.contains_dependencies())
        {
            deps.collect_unique_symbolic_dependencies(missing_symbols);
            for (const auto& symbol : missing_symbols)
                add_diagnostic(
                    diagnostic_op::error_E010("ordinary symbol", symbol.to_string_view(), op->operand_range));
            if (missing_symbols.empty()) // this is a fallback message if somehow non-symbolic deps are not resolved
                add_diagnostic(diagnostic_op::error_E016(op->operand_range));
            else
                missing_symbols.clear();
            continue;
        }

        op->apply_mach_visitor(lc);

        const auto [def_type, exact_match] = check_type_and_extension(*op->value, add_diagnostic);
        if (!exact_match)
            continue;

        const auto check_op = op->get_operand_value(dep_solver, diags);

        if (!def_type->check(check_op, subtype, add_diagnostic))
            continue;

        if (check_op.length.len_type != checking::data_def_length_t::BIT)
        {
            // align to whole byte
            operands_bit_length = round_up(operands_bit_length, 8ULL);

            // enforce data def alignment
            const context::alignment al = def_type->get_alignment(check_op.length.present);

            operands_bit_length = round_up(operands_bit_length, al.boundary * 8ULL);
        }

        operands_bit_length += def_type->get_length(check_op);
    }

    // align to whole byte
    operands_bit_length = round_up(operands_bit_length, 8ULL);

    if (operands_bit_length / 8 > INT32_MAX)
    {
        add_diagnostic(diagnostic_op::error_D029(stmt_range));
    }
}

} // namespace hlasm_plugin::parser_library::checking
