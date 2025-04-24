/*
 * Copyright (c) 2025 Broadcom.
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

#ifndef PROCESSING_HANDLER_MAP_H
#define PROCESSING_HANDLER_MAP_H

#include <algorithm>
#include <array>
#include <cassert>
#include <utility>

#include "context/compressed_id.h"
#include "utils/projectors.h"

namespace hlasm_plugin::parser_library::processing {

template<typename Callback, std::size_t n>
requires(std::is_function_v<Callback> && n > 0) class handler_map
{
    std::array<context::compressed_id, n> ids;
    std::array<Callback*, n> handlers;

public:
    constexpr Callback* find(context::id_index id) const noexcept
    {
        if (!context::compressed_id::can_compress(id))
            return nullptr;
        const auto it = std::ranges::find(ids, context::compressed_id(id));
        if (it == ids.end())
            return nullptr;

        return handlers[it - ids.begin()];
    }

    explicit constexpr handler_map(const std::pair<context::id_index, Callback*> (&ar)[n]) noexcept
    {
        assert(std::ranges::all_of(ar, context::compressed_id::can_compress, utils::first_element));
        std::ranges::transform(ar, ids.begin(), [](const auto& p) { return context::compressed_id(p.first); });
        std::ranges::transform(ar, handlers.begin(), utils::second_element);
    }
};

template<typename Callback, std::size_t n>
requires(std::is_function_v<Callback> && n > 0)
constexpr auto make_handler_map(const std::pair<context::id_index, Callback*> (&ar)[n]) noexcept
{
    return handler_map<Callback, n>(ar);
}

} // namespace hlasm_plugin::parser_library::processing
#endif
