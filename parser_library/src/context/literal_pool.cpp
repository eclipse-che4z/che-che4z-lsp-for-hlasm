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

#include <functional>

namespace hlasm_plugin::parser_library::context {

id_index literal_pool::add_literal(std::string literal_text, expressions::data_definition dd)
{
    return &m_literals
                .emplace(
                    literal_definition { std::move(literal_text), m_current_literal_pool_generation, std::move(dd) })
                .first->text;
}

void literal_pool::generate_pool() { ++m_current_literal_pool_generation; }

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
