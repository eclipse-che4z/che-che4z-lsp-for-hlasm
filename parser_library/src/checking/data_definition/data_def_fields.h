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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H
#define HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H

#include <cstdint>
#include <variant>
#include <vector>

#include "range.h"

// This file contains definitions of classes that represent
// parts that are used in checking::data_definition_operand

namespace hlasm_plugin::parser_library::checking {

// Represents data def modifiers, type, extension and duplication factor
template<typename T>
struct data_def_field
{
    data_def_field()
        : present(false)
        , value()
        , rng()
    {}
    data_def_field(T value)
        : present(true)
        , value(std::move(value))
        , rng()
    {}
    data_def_field(bool present, T value, range rng)
        : present(present)
        , value(std::move(value))
        , rng(rng)
    {}
    bool present;
    T value;
    range rng;
};
// Specifies whether expression was absolute, relocatable or complex relocatable.
enum class expr_type
{
    ABS,
    RELOC,
    COMPLEX
};
// Represents an expression in nominal value of data definition operand.
struct data_def_expr
{
    int32_t value;
    expr_type ex_kind;
    range rng;
    // When ignored is true, the expression should be ignored by checker.
    bool ignored = false;
};
// Represents the length modifier, adds length type.
struct data_def_length_t : data_def_field<int32_t>
{
    enum length_type
    {
        BYTE,
        BIT
    };

    data_def_length_t()
        : len_type(length_type::BYTE)
    {}
    data_def_length_t(data_def_field<int32_t> field)
        : data_def_field<int32_t>(std::move(field))
        , len_type(length_type::BYTE)
    {}

    length_type len_type;
};
// Represents values of data definition operand written in form D(B).
struct data_def_address
{
    data_def_field<int32_t> base;
    data_def_field<int32_t> displacement;
    bool ignored = false;
    range total;
};

using expr_or_address = std::variant<data_def_expr, data_def_address>;
using nominal_value_expressions = std::vector<expr_or_address>;
using nominal_value_t = data_def_field<std::variant<std::string, nominal_value_expressions>>;
using reduced_nominal_value_t = data_def_field<std::variant<std::string_view, size_t>>;
using scale_modifier_t = data_def_field<int16_t>;
using exponent_modifier_t = data_def_field<int32_t>;
using dupl_factor_modifier_t = data_def_field<int32_t>;

template<typename T>
inline T round_up(T n, T m)
{
    return ((n + m - 1) / m) * m;
}

inline reduced_nominal_value_t reduce_nominal_value(const nominal_value_t& n)
{
    struct
    {
        std::variant<std::string_view, size_t> operator()(const std::string& s) const { return s; }
        std::variant<std::string_view, size_t> operator()(const nominal_value_expressions& e) const { return e.size(); }
    } visitor;
    return reduced_nominal_value_t(n.present, std::visit(visitor, n.value), n.rng);
}
reduced_nominal_value_t reduce_nominal_value(nominal_value_t&& n) = delete;

} // namespace hlasm_plugin::parser_library::checking

#endif // !HLASMPLUGIN_PARSERLIBRARY_CHECKING_DATA_DEF_FIELDS_H
