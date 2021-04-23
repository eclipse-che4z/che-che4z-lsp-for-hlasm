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
#include "postponed_statement_impl.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::processing {

mach_processor::mach_processor(analyzing_context ctx,
    branching_provider& branch_provider,
    workspaces::parse_lib_provider& lib_provider,
    statement_fields_parser& parser)
    : low_language_processor(std::move(ctx), branch_provider, lib_provider, parser)
{}

void mach_processor::process(context::shared_stmt_ptr stmt)
{
    auto rebuilt_stmt = preprocess(stmt);

    auto mnem_tmp = context::instruction::mnemonic_codes.find(*stmt->access_resolved()->opcode_ref().value);

    auto tmp = mnem_tmp != context::instruction::mnemonic_codes.end()
        ? context::instruction::machine_instructions.find(mnem_tmp->second.instruction)
        : context::instruction::machine_instructions.find(*stmt->access_resolved()->opcode_ref().value);

    assert(tmp != context::instruction::machine_instructions.end());

    auto& [name, instr] = *tmp;

    auto label_name = find_label_symbol(rebuilt_stmt);

    if (label_name != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label_name))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", rebuilt_stmt.label_ref().field_range));
        }
        else
        {
            auto addr = hlasm_ctx.ord_ctx.align(context::halfword);

            create_symbol(rebuilt_stmt.stmt_range_ref(),
                label_name,
                addr,
                context::symbol_attributes::make_machine_attrs(
                    (context::symbol_attributes::len_attr)instr.size_for_alloc / 8));
        }
    }

    bool has_dependencies = false;
    for (auto& op : rebuilt_stmt.operands_ref().value)
    {
        auto evaluable = dynamic_cast<semantics::evaluable_operand*>(op.get());
        if (evaluable)
        {
            if (evaluable->has_dependencies(hlasm_ctx.ord_ctx))
            {
                has_dependencies = true;
                break;
            }
        }
    }

    if (has_dependencies)
        hlasm_ctx.ord_ctx.symbol_dependencies.add_dependency(
            std::make_unique<postponed_statement_impl>(std::move(rebuilt_stmt), hlasm_ctx.processing_stack()));
    else
        check(rebuilt_stmt, hlasm_ctx, checker, *this);

    (void)hlasm_ctx.ord_ctx.reserve_storage_area(instr.size_for_alloc / 8, context::halfword);
}

} // namespace hlasm_plugin::parser_library::processing
