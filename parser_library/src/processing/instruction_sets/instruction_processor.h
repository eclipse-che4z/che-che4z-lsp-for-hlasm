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

#include "analyzing_context.h"
#include "diagnosable_ctx.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"
#include "processing/statement.h"

namespace hlasm_plugin::parser_library::processing {
class branching_provider;
class parser_library;

// interface for processing instructions
// processing is divided into classes for assembler, conditional assembly, machine, macro instruction processing
class instruction_processor
{
    virtual void process(std::shared_ptr<const processing::resolved_statement> stmt) = 0;

protected:
    analyzing_context ctx;
    context::hlasm_context& hlasm_ctx;
    branching_provider& branch_provider;
    parse_lib_provider& lib_provider;
    library_info_transitional lib_info;
    expressions::evaluation_context eval_ctx;
    diagnosable_ctx& diag_ctx;

    instruction_processor(const analyzing_context& ctx,
        branching_provider& branch_provider,
        parse_lib_provider& lib_provider,
        diagnosable_ctx& diag_ctx)
        : ctx(ctx)
        , hlasm_ctx(*ctx.hlasm_ctx)
        , branch_provider(branch_provider)
        , lib_provider(lib_provider)
        , lib_info(lib_provider)
        , eval_ctx { *ctx.hlasm_ctx, lib_info, diag_ctx }
        , diag_ctx(diag_ctx)
    {}
    ~instruction_processor() = default;

    void register_literals(
        const processing::resolved_statement& stmt, context::alignment loctr_alignment, size_t unique_id);

    void add_diagnostic(diagnostic_op d) const { diag_ctx.add_diagnostic(std::move(d)); }
};

} // namespace hlasm_plugin::parser_library::processing
#endif
