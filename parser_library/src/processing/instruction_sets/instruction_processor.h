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

#ifndef PROCESSING_INSTRUCTION_PROCESSOR_H
#define PROCESSING_INSTRUCTION_PROCESSOR_H

#include <functional>
#include <unordered_map>

#include "diagnosable_ctx.h"
#include "expressions/evaluation_context.h"
#include "processing/branching_provider.h"
#include "processing/statement.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin::parser_library::processing {

// interface for processing instructions
// processing is divided into classes for assembler, conditional assembly, machine, macro instruction processing
class instruction_processor : public diagnosable_ctx
{
    virtual void process(std::shared_ptr<const processing::resolved_statement> stmt) = 0;

    void collect_diags() const override {}

protected:
    analyzing_context ctx;
    context::hlasm_context& hlasm_ctx;
    branching_provider& branch_provider;
    workspaces::parse_lib_provider& lib_provider;

    expressions::evaluation_context eval_ctx;

    instruction_processor(
        analyzing_context ctx, branching_provider& branch_provider, workspaces::parse_lib_provider& lib_provider)
        : diagnosable_ctx(*ctx.hlasm_ctx)
        , ctx(ctx)
        , hlasm_ctx(*ctx.hlasm_ctx)
        , branch_provider(branch_provider)
        , lib_provider(lib_provider)
        , eval_ctx { *ctx.hlasm_ctx, lib_provider, *this }
    {}

    void register_literals(
        const semantics::complete_statement& stmt, context::alignment loctr_alignment, size_t unique_id);
};

} // namespace hlasm_plugin::parser_library::processing
#endif
