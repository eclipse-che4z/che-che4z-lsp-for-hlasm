/*
 * Copyright (c) 2019 Broadcom.
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

#ifndef HLASMPLUGIN_PARSERLIBRARY_RANGE_H
#define HLASMPLUGIN_PARSERLIBRARY_RANGE_H

// This file contains definitions of LSP representation of
// positions in files and selection ranges.

#include <compare>
#include <string>

#include "parser_library_export.h"

namespace hlasm_plugin::parser_library {

struct PARSER_LIBRARY_EXPORT position
{
    static constexpr const size_t max_value = (size_t)2147483647;
    constexpr position() = default;
    constexpr position(size_t line, size_t column)
        : line(line)
        , column(column)
    {}
    auto operator<=>(const position& oth) const noexcept = default;
    size_t line = 0;
    size_t column = 0;

    static inline position min(const position& lhs, const position& rhs)
    {
        if (lhs.line == rhs.line)
            return position(lhs.line, std::min(lhs.column, rhs.column));
        else
            return (lhs.line < rhs.line) ? lhs : rhs;
    }
    static inline position max(const position& lhs, const position& rhs)
    {
        if (lhs.line == rhs.line)
            return position(lhs.line, std::max(lhs.column, rhs.column));
        else
            return (lhs.line > rhs.line) ? lhs : rhs;
    }
};

struct PARSER_LIBRARY_EXPORT range
{
    range() = default;
    range(position start, position end)
        : start(start)
        , end(end)
    {}
    explicit range(position start)
        : start(start)
        , end(start)
    {}
    bool operator==(const range& r) const { return start == r.start && end == r.end; }
    bool operator!=(const range& r) const { return !(*this == r); }
    position start;
    position end;
};

inline range union_range(const range& lhs, const range& rhs)
{
    return range(position::min(lhs.start, rhs.start), position::max(lhs.end, rhs.end));
}

struct PARSER_LIBRARY_EXPORT file_range
{
    file_range(range r, const std::string* file)
        : r(r)
        , file(file)
    {}
    file_range(range r)
        : r(r)
        , file(nullptr)
    {}
    bool operator==(const file_range& fr) const { return r == fr.r && file == fr.file; }
    range r;
    const std::string* file;
};

} // namespace hlasm_plugin::parser_library
#endif
