/*
 * Copyright (c) 2023 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_BRANCH_INFO_H
#define HLASMPLUGIN_PARSERLIBRARY_BRANCH_INFO_H

#include <cstddef>

namespace hlasm_plugin::parser_library {

enum class branch_direction : unsigned char
{
    none = 0,

    up = 1,
    down = 2,
    somewhere = 4,
};
constexpr branch_direction operator|(branch_direction l, branch_direction r)
{
    return (branch_direction)((unsigned char)l | (unsigned char)r);
}
constexpr branch_direction operator&(branch_direction l, branch_direction r)
{
    return (branch_direction)((unsigned char)l & (unsigned char)r);
}

struct branch_info
{
    constexpr branch_info(std::size_t line, unsigned char col, branch_direction dir)
        : line(line)
        , dir(dir)
        , col(col)
    {}

    std::size_t line;
    branch_direction dir;
    unsigned char col;

    constexpr bool operator==(const branch_info&) const noexcept = default;
};


} // namespace hlasm_plugin::parser_library

#endif
