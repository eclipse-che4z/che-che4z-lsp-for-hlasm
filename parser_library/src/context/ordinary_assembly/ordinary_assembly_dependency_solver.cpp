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

#include "context/literal_pool.h"

namespace hlasm_plugin::parser_library::context {

const symbol* ordinary_assembly_dependency_solver::get_symbol(id_index name) const
{
    auto tmp = ord_context.symbols_.find(name);

    return tmp == ord_context.symbols_.end() ? nullptr : &tmp->second;
}

std::optional<address> ordinary_assembly_dependency_solver::get_loctr() const { return loctr_addr; }

id_index ordinary_assembly_dependency_solver::get_literal_id(const std::string& text,
    const std::shared_ptr<const expressions::data_definition>& lit,
    const range& r,
    bool align_on_halfword)
{
    if (allow_adding_literals)
        return ord_context.m_literals->add_literal(text, lit, r, unique_id, get_loctr(), align_on_halfword);
    else
        return ord_context.m_literals->get_literal(literal_pool_generation, lit, unique_id);
}


dependency_evaluation_context ordinary_assembly_dependency_solver::derive_current_dependency_evaluation_context() const
{
    return dependency_evaluation_context {
        loctr_addr,
        literal_pool_generation,
        unique_id,
    };
}

} // namespace hlasm_plugin::parser_library::context