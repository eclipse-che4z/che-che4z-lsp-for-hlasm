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

#include "context/hlasm_context.h"
#include "context/instruction.h"
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::processing {

mach_processor::mach_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    const processing_manager& proc_mgr)
    : low_language_processor(ctx, branch_provider, lib_provider, parser, proc_mgr)
{}

void mach_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    const auto opcode = stmt->opcode_ref().value;

    auto rebuilt_stmt = preprocess(std::move(stmt));

    register_literals(rebuilt_stmt, context::halfword, hlasm_ctx.ord_ctx.next_unique_id());

    auto loctr = hlasm_ctx.ord_ctx.align(context::halfword, lib_info);

    const auto [mach_instr, _] = context::instruction::find_machine_instruction_or_mnemonic(opcode.to_string_view());
    assert(mach_instr);

    auto label_name = find_label_symbol(rebuilt_stmt);

    if (!label_name.empty())
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
                    (context::symbol_attributes::len_attr)mach_instr->size_in_bits() / 8));
        }
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    hlasm_ctx.ord_ctx.symbol_dependencies().add_dependency(
        std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context(),
        lib_info);

    (void)hlasm_ctx.ord_ctx.reserve_storage_area(mach_instr->size_in_bits() / 8, context::halfword, lib_info);
}

} // namespace hlasm_plugin::parser_library::processing
