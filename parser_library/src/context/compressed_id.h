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

#ifndef CONTEXT_COMPRESSED_ID_H
#define CONTEXT_COMPRESSED_ID_H

#include <bit>
#include <cassert>

#include "id_index.h"

namespace hlasm_plugin::parser_library::context {

class compressed_id
{
    unsigned long long value = 0;

    explicit constexpr compressed_id(unsigned long long v) noexcept
        : value(v)
    {}

    static_assert(sizeof(value) <= sizeof(id_index::m_buffer));

public:
    static constexpr bool can_compress(const id_index& id) noexcept
    {
        return id.m_buffer[sizeof(id.m_buffer) - 1] < sizeof(value);
    }

    static constexpr compressed_id compress(const id_index& id) noexcept
    {
        assert(can_compress(id));

        unsigned char buffer[sizeof(value)]; // C++26 [[indeterminate]];

        std::copy(id.m_buffer, id.m_buffer + sizeof(buffer), buffer);
        buffer[sizeof(buffer) - 1] = id.m_buffer[sizeof(id.m_buffer) - 1];

        return compressed_id(std::bit_cast<unsigned long long>(buffer));
    }

    constexpr compressed_id() noexcept = default;
    explicit constexpr compressed_id(const id_index& id) noexcept
        : compressed_id(compress(id))
    {}

    constexpr bool operator==(const compressed_id& o) const noexcept = default;
};
} // namespace hlasm_plugin::parser_library::context

#endif
