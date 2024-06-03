/*
 * Copyright (c) 2023 Broadcom.
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

#include "diagnostic_tools.h"

#include <format>
#include <string>
#include <utility>

#include "diagnostic.h"
#include "diagnostic_op.h"

namespace hlasm_plugin::parser_library {

diagnostic add_stack_details(diagnostic_op d, context::processing_stack_t stack)
{
    if (stack.empty())
        return std::move(d).to_diagnostic();

    auto diag = std::move(d).to_diagnostic(stack.frame().resource_loc->get_uri());

    for (stack = stack.parent(); !stack.empty(); stack = stack.parent())
    {
        const auto& f = stack.frame();
        diag.related.emplace_back(range_uri(f.resource_loc->get_uri(), range(f.pos)),
            std::format("While compiling {}({})", f.resource_loc->to_presentable(), f.pos.line + 1));
    }

    return diag;
}

} // namespace hlasm_plugin::parser_library
