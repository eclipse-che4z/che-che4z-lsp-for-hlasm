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

#include "macro_param.h"

#include "system_variable.h"

using namespace hlasm_plugin::parser_library::context;

const keyword_param* macro_param_base::access_keyword_param() const
{
    return (param_type == macro_param_type::KEY_PAR_TYPE) ? static_cast<const keyword_param*>(this) : nullptr;
}

const positional_param* macro_param_base::access_positional_param() const
{
    return (param_type == macro_param_type::POS_PAR_TYPE) ? static_cast<const positional_param*>(this) : nullptr;
}

const system_variable* macro_param_base::access_system_variable() const
{
    return (param_type == macro_param_type::SYSTEM_TYPE) ? static_cast<const system_variable*>(this) : nullptr;
}

macro_param_base::macro_param_base(macro_param_type param_type, id_index name, bool is_global)
    : variable_symbol(variable_kind::MACRO_VAR_KIND, name, is_global)
    , param_type(param_type)
{}

const C_t& macro_param_base::get_value(const std::vector<size_t>& offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx - 1);
    }
    return tmp->get_value();
}

const C_t& macro_param_base::get_value(size_t idx) const { return real_data()->get_ith(idx - 1)->get_value(); }

const C_t& macro_param_base::get_value() const { return real_data()->get_value(); }

const macro_param_data_component* macro_param_base::get_data(const std::vector<size_t>& offset) const
{
    auto data = real_data();
    for (auto idx : offset)
    {
        data = data->get_ith(idx - 1);
    }
    return data;
}

A_t macro_param_base::number(std::vector<size_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx - 1);
    }
    return (A_t)tmp->number;
}

A_t macro_param_base::count(std::vector<size_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx - 1);
    }
    return (A_t)tmp->get_value().size();
}

size_t macro_param_base::size(std::vector<size_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (auto idx : offset)
    {
        tmp = tmp->get_ith(idx - 1);
    }
    return tmp->size();
}

keyword_param::keyword_param(id_index name, macro_data_shared_ptr default_value, macro_data_ptr assigned_value)
    : macro_param_base(macro_param_type::KEY_PAR_TYPE, name, false)
    , assigned_data_(std::move(assigned_value))
    , default_data(std::move(default_value))
{}

const macro_param_data_component* keyword_param::real_data() const
{
    return assigned_data_ ? assigned_data_.get() : default_data.get();
}

positional_param::positional_param(id_index name, size_t position, const macro_param_data_component& assigned_value)
    : macro_param_base(macro_param_type::POS_PAR_TYPE, name, false)
    , data_(assigned_value)
    , position(position)
{}

const macro_param_data_component* positional_param::real_data() const { return &data_; }
