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

#include "system_variable.h"

#include <memory>

#include "diagnostic.h"
#include "diagnostic_consumer.h"
#include "range.h"
#include "utils/unicode_text.h"

using namespace hlasm_plugin::parser_library::context;


system_variable::system_variable(id_index name, macro_data_ptr value, bool is_global)
    : macro_param_base(macro_param_type::SYSTEM_TYPE, name, is_global)
    , data_(std::move(value))
{}

C_t system_variable::get_value(std::span<const A_t> offset) const { return get_data(offset)->get_value(); }

C_t system_variable::get_value(A_t idx) const { return macro_param_base::get_value(idx); }

C_t system_variable::get_value() const { return macro_param_base::get_value(0); }

const macro_param_data_component* system_variable::get_data(std::span<const A_t> offset) const
{
    for (auto subscript : offset)
    {
        if (1 != subscript)
        {
            return macro_param_data_component::dummy.get();
        }
    }

    return data_->get_ith(1);
}

A_t system_variable::number(std::span<const A_t> offset) const
{
    if (offset.empty())
        return data_->number - 1;
    else
        return macro_param_base::number(offset);
}

A_t system_variable::count(std::span<const A_t> offset) const
{
    if (offset.empty())
        return (A_t)utils::length_utf32_no_validation(data_->get_ith(1)->get_value());

    const macro_param_data_component* tmp = real_data();
    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx);
    }
    return (A_t)utils::length_utf32_no_validation(tmp->get_value());
}

std::optional<std::pair<A_t, A_t>> system_variable::index_range(std::span<const A_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx);
    }

    return tmp->index_range();
}

const macro_param_data_component* system_variable::real_data() const { return std::to_address(data_); }

C_t system_variable_sysmac::get_value(std::span<const A_t> offset) const
{
    if (!offset.empty())
        return get_data(offset)->get_value();
    else
        return get_data(std::array<A_t, 1> { 0 })->get_value();
}

C_t system_variable_sysmac::get_value(A_t idx) const { return system_variable::get_value(idx); }

C_t system_variable_sysmac::get_value() const { return system_variable::get_value(); }

const macro_param_data_component* system_variable_sysmac::get_data(std::span<const A_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    if (!offset.empty())
        tmp = tmp->get_ith(offset.back()); // what the original seems to do

    return tmp;
}

bool system_variable_sysmac::can_read(std::span<const A_t>, range, diagnostic_consumer<diagnostic_op>&) const
{
    return true;
}

const macro_param_data_component* system_variable_syslist::get_data(std::span<const A_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    if (offset.empty())
    {
        tmp = tmp->get_ith(1);
    }
    else
    {
        for (auto idx : offset)
        {
            tmp = tmp->get_ith(idx);
        }
    }

    return tmp;
}

bool system_variable_syslist::can_read(
    std::span<const A_t> subscript, range symbol_range, diagnostic_consumer<diagnostic_op>& diags) const
{
    if (subscript.empty())
    {
        diags.add_diagnostic(diagnostic_op::error_E076(symbol_range)); // error - SYSLIST is not subscripted
    }

    for (size_t i = 0; i < subscript.size(); ++i)
    {
        if (subscript[i] < 1)
        {
            // if subscript = 0, ok
            if (i == 0 && subscript[i] == 0)
                continue;

            diags.add_diagnostic(diagnostic_op::error_E012(
                "subscript value has to be 1 or more", symbol_range)); // error - subscript is less than 1
            return false;
        }
    }

    return true;
}
