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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H

// This file contains C++ representation of DAP types
// used to present the context of HLASM analysis to
// the user.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

namespace hlasm_plugin::parser_library::debugging {

using frame_id_t = size_t;
using var_reference_t = size_t;

enum class set_type
{
    A_TYPE,
    B_TYPE,
    C_TYPE,
    UNDEF_TYPE
};

// Interface that represents a variable to be shown to the user
// through DAP.
struct variable
{
    std::string name;
    std::string value;
    set_type type = set_type::UNDEF_TYPE;
    var_reference_t var_reference = 0;

    std::function<std::vector<variable>()> values;

    bool is_scalar() const noexcept { return !values; }
};

struct breakpoint
{
    explicit breakpoint(size_t line)
        : line(line)
    {}
    size_t line;
};

struct function_breakpoint
{
    explicit function_breakpoint(std::string_view name)
        : name(name)
    {}
    std::string_view name;
};

struct source
{
    explicit source(std::string uri)
        : uri(std::move(uri))
    {}
    std::string uri;
    bool operator==(const source& oth) const noexcept = default;
};

struct stack_frame
{
    explicit stack_frame(size_t begin_line, size_t end_line, uint32_t id, std::string name, source source)
        : begin_line(begin_line)
        , end_line(end_line)
        , id(id)
        , name(std::move(name))
        , frame_source(std::move(source))
    {}
    size_t begin_line;
    size_t end_line;
    uint32_t id;
    std::string name;
    source frame_source;
};

struct scope
{
    explicit scope(std::string name, var_reference_t ref, source source)
        : name(std::move(name))
        , scope_source(std::move(source))
        , var_reference(ref)
    {}
    std::string name;
    source scope_source;
    var_reference_t var_reference;
};

struct evaluated_expression
{
    std::string result;
    bool error = false;
    std::size_t var_ref = 0;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_STACK_FRAME_H
