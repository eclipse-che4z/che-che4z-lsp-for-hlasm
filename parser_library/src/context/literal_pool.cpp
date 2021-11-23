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

#include "context/ordinary_assembly/ordinary_assembly_context.h"
#include "semantics/operand_impls.h"

namespace hlasm_plugin::parser_library::context {

id_index literal_pool::add_literal(const std::string& literal_text,
    const std::shared_ptr<const expressions::data_definition>& dd,
    location loc,
    size_t unique_id)
{
    unique_id = dd->references_loctr ? unique_id : 0;
    if (auto lit = get_literal(m_current_literal_pool_generation, dd, unique_id))
        return lit;

    // TODO: processing stack
    auto [it, inserted] = m_literals.emplace(
        literal_definition { literal_text, m_current_literal_pool_generation, unique_id, dd, std::move(loc), {} });
    // even if we end up inserting a duplicate
    // we need to try to insert const expressions::data_definition->iterator relation
    // because a single literal may be referenced by independent data_definitions
    m_literals_genmap.try_emplace(literal_id { m_current_literal_pool_generation, unique_id, dd.get() }, it);
    // but we should not try to put logical duplicates on the pending queue
    if (inserted)
        m_pending_literals.emplace_back(&*it);

    return &it->text;
}

id_index literal_pool::get_literal(
    size_t generation, const std::shared_ptr<const expressions::data_definition>& dd, size_t unique_id) const
{
    unique_id = dd->references_loctr ? unique_id : 0;
    auto it = m_literals_genmap.find(literal_id { generation, unique_id, dd.get() });
    if (it == m_literals_genmap.end())
        return nullptr;
    return &it->second->text;
}

void literal_pool::generate_pool(ordinary_assembly_context& ord_ctx, dependency_solver& solver)
{
    for (auto& [lit, size, alignment] : m_pending_literals)
    {
        auto* type = lit->value->access_data_def_type();
        if (!type)
            continue;

        size = (semantics::data_def_operand::get_operand_value(*lit->value, solver).get_length() + 7) / 8;
        if (size == 0)
            continue;

        auto top_alignment = size | 16; // 16B length alignment is the top
        alignment = (~top_alignment & top_alignment - 1) + 1;
    }

    std::stable_sort(m_pending_literals.begin(), m_pending_literals.end(), [](const auto& l, const auto& r) {
        return l.alignment > r.alignment;
    });

    // TODO: Generate all literals
    constexpr auto sectalign = doubleword;
    ord_ctx.align(sectalign);
    for (auto& [lit, size, alignment] : m_pending_literals)
    {
        if (size == 0)
            break;
        auto addr = ord_ctx.reserve_storage_area(size, no_align);
        symbol_attributes attrs(
            symbol_origin::EQU, lit->value->get_type_attribute(), lit->value->get_length_attribute(solver));
        bool cycle = ord_ctx.create_symbol(&lit->text, addr, attrs, {});
    }

    m_pending_literals.clear();
    ++m_current_literal_pool_generation;
}

bool literal_pool::literal_definition::is_similar(const literal_definition& ld) const noexcept
{
    return generation == ld.generation && unique_id == ld.unique_id && text == ld.text; // for now
}

size_t literal_pool::literal_definition_hasher::operator()(const literal_definition& ld) const noexcept
{
    auto text_hash = std::hash<std::string> {}(ld.text);
    return text_hash ^ ld.generation ^ ld.unique_id;
}

} // namespace hlasm_plugin::parser_library::context
