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

#include "dependency_solver_redirect.h"

#include "context/using.h"

namespace hlasm_plugin::parser_library::context {

const symbol* dependency_solver_redirect::get_symbol(id_index name) const { return m_base->get_symbol(name); }

std::optional<address> dependency_solver_redirect::get_loctr() const { return m_base->get_loctr(); }

id_index dependency_solver_redirect::get_literal_id(const std::shared_ptr<const expressions::data_definition>& lit)
{
    return m_base->get_literal_id(lit);
}

bool dependency_solver_redirect::using_active(id_index label, const section* sect) const
{
    return m_base->using_active(label, sect);
}

using_evaluate_result dependency_solver_redirect::using_evaluate(
    id_index label, const section* owner, int32_t offset, bool long_offset) const
{
    return m_base->using_evaluate(label, owner, offset, long_offset);
}

std::variant<const symbol*, symbol_candidate> dependency_solver_redirect::get_symbol_candidate(id_index name) const
{
    return m_base->get_symbol_candidate(name);
}

std::string dependency_solver_redirect::get_opcode_attr(id_index symbol) const
{
    return m_base->get_opcode_attr(symbol);
}

} // namespace hlasm_plugin::parser_library::context
