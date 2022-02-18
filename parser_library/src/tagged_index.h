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

#ifndef HLASMPLUGIN_PARSERLIBRARY_TAGGED_INDEX_H
#define HLASMPLUGIN_PARSERLIBRARY_TAGGED_INDEX_H

#include <cstddef>

namespace hlasm_plugin::parser_library {

template<typename Tag>
class index_t
{
    std::size_t index = 0;

public:
    index_t() = default;
    constexpr index_t(std::size_t i) noexcept
        : index(i)
    {
        assert(i != 0);
    }

    friend bool operator==(index_t l, index_t r) = default;

    constexpr explicit operator bool() const { return index != 0; }

    constexpr std::size_t value() const
    {
        assert(index != 0);
        return index;
    }
};

} // namespace hlasm_plugin::parser_library

#endif