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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TAGGED_INDEX_H
#define HLASMPLUGIN_PARSERLIBRARY_TAGGED_INDEX_H

#include <cassert>
#include <concepts>
#include <cstddef>

namespace hlasm_plugin::parser_library {

template<typename Tag, /* std::integral */ typename T = std::size_t>
class index_t
{
    static constexpr T invalid_index = (T)-1;
    T index = invalid_index;

public:
    index_t() = default;
    explicit constexpr index_t(T i) noexcept
        : index(i)
    {
        assert(i != invalid_index);
    }

    friend bool operator==(index_t l, index_t r) = default;

    constexpr explicit operator bool() const { return index != invalid_index; }

    constexpr T value() const
    {
        assert(index != invalid_index);
        return index;
    }
};

} // namespace hlasm_plugin::parser_library

#endif
