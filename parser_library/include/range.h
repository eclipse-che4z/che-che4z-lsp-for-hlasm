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

#include <string>

#include "parser_library_export.h"

namespace hlasm_plugin {
namespace parser_library {

using position_t = uint64_t;

struct PARSER_LIBRARY_EXPORT position
{
    position()
        : line(0)
        , column(0)
    {}
    position(position_t line, position_t column)
        : line(line)
        , column(column)
    {}
    bool operator==(const position& oth) const { return line == oth.line && column == oth.column; }
    bool operator!=(const position& oth) const { return !(*this == oth); }
    position_t line;
    position_t column;
};

struct PARSER_LIBRARY_EXPORT range
{
    range() {}
    range(position start, position end)
        : start(start)
        , end(end)
    {}
    explicit range(position start)
        : start(start)
        , end(start)
    {}
    bool operator==(const range& r) const { return start == r.start && end == r.end; }
    position start;
    position end;
};

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

struct PARSER_LIBRARY_EXPORT location
{
    location() {}
    location(position pos, std::string file)
        : pos(pos)
        , file(file)
    {}
    bool operator==(const location& oth) const { return pos == oth.pos && file == oth.file; }
    position pos;
    std::string file;
};

} // namespace parser_library
} // namespace hlasm_plugin
#endif
