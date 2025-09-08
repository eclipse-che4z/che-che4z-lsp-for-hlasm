/*
 * Copyright (c) 2022 Broadcom.
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

#include "instruction_processor.h"

#include "context/hlasm_context.h"
#include "context/literal_pool.h"

namespace hlasm_plugin::parser_library::processing {

void instruction_processor::register_literals(
    const processing::resolved_statement& stmt, context::alignment loctr_alignment, size_t unique_id)
{
    if (auto literals = stmt.literals(); !literals.empty())
    {
        auto loctr = hlasm_ctx.ord_ctx.align(loctr_alignment);
        for (const auto& l : literals)
            hlasm_ctx.ord_ctx.literals().add_literal(l->get_text(),
                std::shared_ptr<const expressions::data_definition>(l, &l->get_dd()),
                l->get_range(),
                unique_id,
                loctr,
                l->get_referenced_by_reladdr());
    }
}
} // namespace hlasm_plugin::parser_library::processing
