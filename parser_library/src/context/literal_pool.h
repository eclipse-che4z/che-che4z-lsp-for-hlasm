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
        expressions::data_definition value;

        bool is_similar(const literal_definition&) const noexcept;
    };
    struct literal_definition_hasher
    {
        size_t operator()(const literal_definition&) const noexcept;
    };
    size_t m_current_literal_pool_generation = 0;

    std::unordered_set<literal_definition, literal_definition_hasher, decltype(utils::is_similar)> m_literals;

public:
    id_index add_literal(std::string literal_text, expressions::data_definition dd);

    void generate_pool();
};

} // namespace hlasm_plugin::parser_library::context

#endif