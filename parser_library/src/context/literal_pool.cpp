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

#include "literal_pool.h"

#include <algorithm>
#include <functional>

#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::context {

id_index literal_pool::add_literal(
    const std::string& literal_text, const std::shared_ptr<const expressions::data_definition>& dd)
{
    if (auto lit = get_literal(m_current_literal_pool_generation, dd))
        return lit;

    auto it = m_literals.emplace(literal_definition { literal_text, m_current_literal_pool_generation, dd }).first;
    m_literals_genmap.emplace(std::make_pair(m_current_literal_pool_generation, dd.get()), it);
    m_pending_literals.emplace_back(&*it, 0);

    return &it->text;
}

id_index literal_pool::get_literal(
    size_t generation, const std::shared_ptr<const expressions::data_definition>& dd) const
{
    auto it = m_literals_genmap.find(std::make_pair(generation, dd.get()));
    if (it == m_literals_genmap.end())
        return nullptr;
    return &it->second->text;
}

void literal_pool::generate_pool(dependency_solver& solver)
{
    for (auto& [lit, alignment] : m_pending_literals)
    {
        auto* type = lit->value->access_data_def_type();
        if (!type)
            continue;

        auto length = (semantics::data_def_operand::get_operand_value(*lit->value, solver).get_length() + 7) / 8;
        if (length == 0)
            continue;

        auto top_alignment = length | 16; // 16B length alignment is the top
        top_alignment = ~top_alignment & top_alignment - 1;
        alignment = top_alignment + 1;
    }

    std::stable_sort(m_pending_literals.begin(), m_pending_literals.end(), [](const auto& l, const auto& r) {
        return l.second > r.second;
    });

    // TODO: Generate all literals

    ++m_current_literal_pool_generation;
}

bool literal_pool::literal_definition::is_similar(const literal_definition& ld) const noexcept
{
    return generation == ld.generation && text == ld.text; // for now
}

size_t literal_pool::literal_definition_hasher::operator()(const literal_definition& ld) const noexcept
{
    auto text_hash = std::hash<std::string> {}(ld.text);
    return text_hash ^ ld.generation;
}

} // namespace hlasm_plugin::parser_library::context
