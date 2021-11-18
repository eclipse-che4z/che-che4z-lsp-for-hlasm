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
#include <unordered_set>

#include "expressions/data_definition.h"
#include "id_storage.h"
#include "utils/similar.h"

namespace hlasm_plugin::parser_library::context {

class literal_pool
{
    struct literal_definition
    {
        std::string text;
        size_t generation;
        std::shared_ptr<const expressions::data_definition> value;

        bool is_similar(const literal_definition&) const noexcept;
    };
    struct literal_definition_hasher
    {
        size_t operator()(const literal_definition&) const noexcept;
    };
    size_t m_current_literal_pool_generation = 0;

    struct pair_hash
    {
        template<typename T1, typename T2>
        size_t operator()(const std::pair<T1, T2>& p) const noexcept
        {
            return std::hash<T1>()(p.first) ^ std::hash<T2>()(p.second);
        }
    };

    std::unordered_set<literal_definition, literal_definition_hasher, decltype(utils::is_similar)> m_literals;
    std::unordered_map<std::pair<size_t, const expressions::data_definition*>,
        decltype(m_literals)::const_iterator,
        pair_hash>
        m_literals_genmap;
    std::vector<std::pair<const literal_definition*, size_t>> m_pending_literals;

public:
    id_index add_literal(
        const std::string& literal_text, const std::shared_ptr<const expressions::data_definition>& dd);
    id_index get_literal(size_t generation, const std::shared_ptr<const expressions::data_definition>& dd) const;

    void generate_pool(dependency_solver& solver);
    size_t current_generation() const { return m_current_literal_pool_generation; }
};

} // namespace hlasm_plugin::parser_library::context

#endif