/*
 * Copyright (c) 2021 Broadcom.
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

#include "diagnosable_ctx.h"

namespace hlasm_plugin::parser_library {

void diagnosable_ctx::add_diagnostic(diagnostic_s diagnostic) const
{
    diagnosable_impl::add_diagnostic(add_stack_details(
        diagnostic_op(
            diagnostic.severity, std::move(diagnostic.code), std::move(diagnostic.message), diagnostic.diag_range),
        ctx_.processing_stack()));
}

void diagnosable_ctx::add_diagnostic(diagnostic_op diagnostic) const
{
    diagnosable_impl::add_diagnostic(add_stack_details(std::move(diagnostic), ctx_.processing_stack()));
}

diagnostic_s add_stack_details(diagnostic_op diagnostic, context::processing_stack_t stack)
{
    diagnostic_s diag(std::move(stack.back().proc_location.file), std::move(diagnostic));

    diag.related.reserve(stack.size() - 1);
    for (auto frame = ++stack.rbegin(); frame != stack.rend(); ++frame)
    {
        auto& file_name = frame->proc_location.file;
        auto message = "While compiling " + file_name + '(' + std::to_string(frame->proc_location.pos.line + 1) + ")";
        diag.related.emplace_back(
            range_uri_s(std::move(file_name), range(frame->proc_location.pos, frame->proc_location.pos)),
            std::move(message));
    }

    return diag;
}

} // namespace hlasm_plugin::parser_library
