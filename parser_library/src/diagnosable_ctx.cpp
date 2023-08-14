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

#include "context/hlasm_context.h"
#include "diagnostic_tools.h"

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

} // namespace hlasm_plugin::parser_library
