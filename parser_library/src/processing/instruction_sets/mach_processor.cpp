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
#include "context/ordinary_assembly/ordinary_assembly_dependency_solver.h"
#include "context/ordinary_assembly/symbol_dependency_tables.h"
#include "instructions/instruction.h"
#include "postponed_statement_impl.h"

namespace hlasm_plugin::parser_library::processing {

mach_processor::mach_processor(const analyzing_context& ctx,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser,
    const processing_manager& proc_mgr,
    diagnosable_ctx& diag_ctx)
    : low_language_processor(ctx, branch_provider, lib_provider, parser, proc_mgr, diag_ctx)
{}

namespace {
size_t op_size(const processing::op_code& op) noexcept
{
    switch (op.type)
    {
        case context::instruction_type::MACH:
            return op.instr_mach->size_in_bits() / 8;
        case context::instruction_type::MNEMO:
            return op.instr_mnemo->size_in_bits() / 8;
        default:
            assert(false);
            return 0;
    }
}
} // namespace

void mach_processor::process(std::shared_ptr<const processing::resolved_statement> stmt)
{
    const auto opcode_size = op_size(stmt->opcode_ref());

    auto rebuilt_stmt = preprocess(std::move(stmt));

    register_literals(rebuilt_stmt, context::halfword, hlasm_ctx.ord_ctx.next_unique_id());

    auto loctr = hlasm_ctx.ord_ctx.align(context::halfword);

    auto label_name = find_label_symbol(rebuilt_stmt);

    if (!label_name.empty())
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label_name))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", rebuilt_stmt.label_ref().field_range));
        }
        else
        {
            create_symbol(label_name,
                loctr,
                context::symbol_attributes::make_machine_attrs((context::symbol_attributes::len_attr)opcode_size));
        }
    }

    context::ordinary_assembly_dependency_solver dep_solver(hlasm_ctx.ord_ctx, std::move(loctr), lib_info);

    hlasm_ctx.ord_ctx.symbol_dependencies().add_postponed_statement(
        std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()),
        std::move(dep_solver).derive_current_dependency_evaluation_context());

    (void)hlasm_ctx.ord_ctx.reserve_storage_area(opcode_size, context::halfword);
}

} // namespace hlasm_plugin::parser_library::processing
