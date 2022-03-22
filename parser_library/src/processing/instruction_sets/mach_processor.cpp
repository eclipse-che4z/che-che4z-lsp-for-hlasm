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

#include "mach_processor.h"

#include <algorithm>

#include "context/instruction_type.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "postponed_statement_impl.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

mach_processor::mach_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider,
    statement_fields_parser& parser)
    : low_language_processor(std::move(ctx), branch_provider, lib_provider, parser)
{}

void mach_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    auto rebuilt_stmt = preprocess(stmt);

    register_literals(rebuilt_stmt, context::halfword, hlasm_ctx.ord_ctx.next_unique_id());

    auto loctr = hlasm_ctx.ord_ctx.align(context::halfword);

    const auto& mach_instr = [](const std::string& name) {
        if (auto mnemonic = context::instruction::find_mnemonic_codes(name))
            return *mnemonic->instruction();
        else
            return context::instruction::get_machine_instructions(name);
    }(*stmt->opcode_ref().value);

    auto label_name = find_label_symbol(rebuilt_stmt);

    if (label_name != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label_name))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", rebuilt_stmt.label_ref().field_range));
        }
        else
        {
            create_symbol(rebuilt_stmt.stmt_range_ref(),
                label_name,
                loctr,
                context::symbol_attributes::make_machine_attrs(
                    (context::symbol_attributes::len_attr)mach_instr.size_in_bits() / 8));
        }
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, loctr);

    hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()),
        dep_solver.derive_current_dependency_evaluation_context());

    (void)hlasm_ctx.ord_ctx.reserve_storage_area(mach_instr.size_in_bits() / 8, context::halfword);
}

} // namespace hlasm_plugin::parser_library::processing
