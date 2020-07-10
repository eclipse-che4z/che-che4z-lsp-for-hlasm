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
#include "parsing/parser_impl.h"
#include "postponed_statement_impl.h"
#include "processing/context_manager.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::processing;
using namespace hlasm_plugin::parser_library::workspaces;

mach_processor::mach_processor(context::hlasm_context& hlasm_ctx,
    attribute_provider& attr_provider,
    branching_provider& branch_provider,
    parse_lib_provider& lib_provider,
    statement_fields_parser& parser)
    : low_language_processor(hlasm_ctx, attr_provider, branch_provider, lib_provider, parser)
{ }

void mach_processor::process(context::shared_stmt_ptr stmt)
{
    process(preprocess(stmt), stmt->access_resolved()->opcode_ref());
}

void mach_processor::process(context::unique_stmt_ptr stmt)
{
    auto opcode = stmt->access_resolved()->opcode_ref();
    process(preprocess(std::move(stmt)), opcode);
}

void mach_processor::process(rebuilt_statement stmt, const op_code& opcode)
{
    auto mnem_tmp = context::instruction::mnemonic_codes.find(*opcode.value);

    auto tmp = mnem_tmp != context::instruction::mnemonic_codes.end()
        ? context::instruction::machine_instructions.find(mnem_tmp->second.instruction)
        : context::instruction::machine_instructions.find(*opcode.value);

    assert(tmp != context::instruction::machine_instructions.end());

    auto& [name, instr] = *tmp;

    auto label_name = find_label_symbol(stmt);

    if (label_name != context::id_storage::empty_id)
    {
        if (hlasm_ctx.ord_ctx.symbol_defined(label_name))
        {
            add_diagnostic(diagnostic_op::error_E031("symbol", stmt.label_ref().field_range));
        }
        else
        {
            auto addr = hlasm_ctx.ord_ctx.align(context::halfword);

            create_symbol(stmt.stmt_range_ref(),
                label_name,
                addr,
                context::symbol_attributes::make_machine_attrs(
                    (context::symbol_attributes::len_attr)instr->size_for_alloc / 8));
        }
    }

    bool has_dependencies = false;
    for (auto& op : stmt.operands_ref().value)
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
            std::make_unique<postponed_statement_impl>(std::move(stmt), hlasm_ctx.processing_stack()));
    else
        check(stmt, hlasm_ctx, checker, *this);

    (void)hlasm_ctx.ord_ctx.reserve_storage_area(instr->size_for_alloc / 8, context::halfword);
}
