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

#include "context/hlasm_context.h"
#include "diagnosable_ctx.h"
#include "expressions/evaluation_context.h"
#include "processing/branching_provider.h"
#include "processing/statement.h"
#include "workspaces/parse_lib_provider.h"

namespace hlasm_plugin {
namespace parser_library {
namespace processing {

// interface for processing instructions
// processing is divided into classes for assembler, conditional assembly, machine, macro instruction processing
class instruction_processor : public diagnosable_ctx
{
    virtual void process(context::shared_stmt_ptr stmt) = 0;

    virtual void collect_diags() const override { collect_diags_from_child(eval_ctx); }

protected:
    context::hlasm_context& hlasm_ctx;
    branching_provider& branch_provider;
    workspaces::parse_lib_provider& lib_provider;

    expressions::evaluation_context eval_ctx;

    instruction_processor(context::hlasm_context& hlasm_ctx,
        branching_provider& branch_provider,
        workspaces::parse_lib_provider& lib_provider)
        : diagnosable_ctx(hlasm_ctx)
        , hlasm_ctx(hlasm_ctx)
        , branch_provider(branch_provider)
        , lib_provider(lib_provider)
        , eval_ctx { hlasm_ctx, lib_provider }
    {}
};

} // namespace processing
} // namespace parser_library
} // namespace hlasm_plugin
#endif
