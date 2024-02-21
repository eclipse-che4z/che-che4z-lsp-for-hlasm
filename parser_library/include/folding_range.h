/*
 * Copyright (c) 2024 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_FOLDING_RANGE_H
#define HLASMPLUGIN_PARSERLIBRARY_FOLDING_RANGE_H

#include <cstddef>

namespace hlasm_plugin::parser_library {

enum class fold_type
{
    none,
    comment,
};

struct folding_range
{
    constexpr folding_range(std::size_t start, std::size_t end, fold_type type)
        : start(start)
        , end(end)
        , type(type)
    {}

    std::size_t start;
    std::size_t end;
    fold_type type;

    constexpr bool operator==(const folding_range&) const noexcept = default;
};


} // namespace hlasm_plugin::parser_library

#endif
