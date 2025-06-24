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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LITERAL_POOL_H
#define HLASMPLUGIN_PARSERLIBRARY_LITERAL_POOL_H

#include <unordered_map>

#include "expressions/data_definition.h"
#include "id_index.h"
#include "source_context.h"
#include "tagged_index.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library {
class diagnosable_ctx;
class library_info;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {
class hlasm_context;
class using_collection;

class literal_pool
{
    struct literal_id
    {
        size_t generation;
        size_t unique_id;
        std::shared_ptr<const expressions::data_definition> lit;

        bool is_similar(const literal_id&) const noexcept;
    };
    struct ca_only_literal
    {};
    struct literal_details
    {
        std::string text;
        range r;
        std::optional<address> loctr;
        processing_stack_t stack;
        bool align_on_halfword = false;
        bool ca_expr_only = false;

        literal_details(std::string text, range r, std::optional<address> loctr)
            : text(std::move(text))
            , r(r)
            , loctr(std::move(loctr))
        {}

        literal_details(std::string text, range r, std::optional<address> loctr, processing_stack_t stack)
            : text(std::move(text))
            , r(r)
            , loctr(std::move(loctr))
            , stack(std::move(stack))
        {}
        explicit literal_details(ca_only_literal)
            : ca_expr_only(true)
        {}
    };
    class literal_postponed_statement;
    struct literal_definition_hasher
    {
        size_t operator()(const literal_id&) const noexcept;
    };
    size_t m_current_literal_pool_generation = 0;

    struct literal_id_helper
    {
        size_t operator()(const literal_id& p) const noexcept
        {
            return std::hash<size_t>()(p.generation) ^ std::hash<size_t>()(p.unique_id)
                ^ std::hash<const expressions::data_definition*>()(p.lit.get());
        }
        bool operator()(const literal_id& l, const literal_id& r) const noexcept
        {
            return l.lit == r.lit && l.generation == r.generation && l.unique_id == r.unique_id;
        }
    };

    std::unordered_map<literal_id, literal_details, literal_definition_hasher, decltype(utils::is_similar)> m_literals;
    std::unordered_map<literal_id, decltype(m_literals)::const_iterator, literal_id_helper, literal_id_helper>
        m_literals_genmap;

    struct pending_literal
    {
        decltype(m_literals)::const_iterator literal;
        size_t size = 0;
        size_t alignment = 0;

        explicit pending_literal(decltype(m_literals)::const_iterator l) noexcept
            : literal(l)
        {}
    };

    std::vector<pending_literal> m_pending_literals;

    hlasm_context& hlasm_ctx;

public:
    explicit literal_pool(hlasm_context& hlasm_ctx)
        : hlasm_ctx(hlasm_ctx)
    {}

    id_index add_literal(const std::string& literal_text,
        const std::shared_ptr<const expressions::data_definition>& dd,
        range r,
        size_t unique_id,
        std::optional<address> loctr,
        bool align_on_halfword);
    id_index get_literal(
        size_t generation, const std::shared_ptr<const expressions::data_definition>& dd, size_t unique_id) const;

    bool defined_for_ca_expr(std::shared_ptr<const expressions::data_definition> dd) const;
    void mentioned_in_ca_expr(std::shared_ptr<const expressions::data_definition> dd);

    void generate_pool(diagnosable_ctx& diags, index_t<using_collection> active_using, const library_info& li);
    size_t current_generation() const { return m_current_literal_pool_generation; }

    // testing
    size_t get_pending_count() const { return m_pending_literals.size(); }
};

} // namespace hlasm_plugin::parser_library::context

#endif
