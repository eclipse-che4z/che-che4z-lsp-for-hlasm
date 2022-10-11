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

#include "set_symbol.h"

#include "diagnostic.h"
#include "diagnostic_consumer.h"
#include "utils/unicode_text.h"

namespace hlasm_plugin::parser_library::context {

bool set_symbol_base::can_read(
    std::span<const A_t> subscript, range symbol_range, diagnostic_consumer<diagnostic_op>& diags) const
{
    if (subscript.size() > 1)
    {
        diags.add_diagnostic(
            diagnostic_op::error_E020("variable symbol subscript", symbol_range)); // error - too many operands
        return false;
    }

    if ((is_scalar && subscript.size() == 1) || (!is_scalar && subscript.empty()))
    {
        diags.add_diagnostic(
            diagnostic_op::error_E013("subscript error", symbol_range)); // error - inconsistent format of subcript
        return false;
    }

    if (!is_scalar && (subscript.front() < 1))
    {
        diags.add_diagnostic(diagnostic_op::error_E012(
            "subscript value has to be 1 or more", symbol_range)); // error - subscript is less than 1
        return false;
    }

    return true;
}

set_symbol_base::set_symbol_base(id_index name, bool is_scalar, bool is_global, SET_t_enum type)
    : variable_symbol(variable_kind::SET_VAR_KIND, name, is_global)
    , is_scalar(is_scalar)
    , type(type)
{}

template<>
A_t set_symbol<A_t>::count(std::span<const size_t> offset) const
{
    auto tmp = get_data(offset);
    return tmp ? (A_t)utils::length_utf32_no_validation(std::to_string(*tmp)) : (A_t)1;
}

template<>
A_t set_symbol<B_t>::count(std::span<const size_t>) const
{
    return (A_t)1;
}

template<>
A_t set_symbol<C_t>::count(std::span<const size_t> offset) const
{
    auto tmp = get_data(offset);
    return tmp ? (A_t)utils::length_utf32_no_validation(*tmp) : (A_t)0;
}
} // namespace hlasm_plugin::parser_library::context
