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

#include "ordinary_assembly_dependency_solver.h"

#include "context/hlasm_context.h"
#include "context/literal_pool.h"
#include "context/using.h"

namespace hlasm_plugin::parser_library::context {

const symbol* ordinary_assembly_dependency_solver::get_symbol(id_index name) const
{
    auto tmp = ord_context.symbols_.find(name);

    return tmp == ord_context.symbols_.end() ? nullptr : std::get_if<symbol>(&tmp->second);
}

std::optional<address> ordinary_assembly_dependency_solver::get_loctr() const { return loctr_addr; }

id_index ordinary_assembly_dependency_solver::get_literal_id(
    const std::shared_ptr<const expressions::data_definition>& lit)
{
    return ord_context.m_literals->get_literal(literal_pool_generation, lit, unique_id);
}


dependency_evaluation_context ordinary_assembly_dependency_solver::derive_current_dependency_evaluation_context() const
{
    return dependency_evaluation_context {
        loctr_addr,
        literal_pool_generation,
        unique_id,
        active_using,
    };
}

bool ordinary_assembly_dependency_solver::using_active(id_index label, const section* sect) const
{
    return ord_context.using_label_active(active_using, label, sect);
}

using_evaluate_result ordinary_assembly_dependency_solver::using_evaluate(
    id_index label, const section* owner, int32_t offset, bool long_offset) const
{
    const auto& u = ord_context.hlasm_ctx_.usings();
    assert(u.resolved());

    return u.evaluate(active_using, label, owner, offset, long_offset);
}

} // namespace hlasm_plugin::parser_library::context