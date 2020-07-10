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

using namespace hlasm_plugin::parser_library::context;


system_variable::system_variable(id_index name, macro_data_ptr value, bool is_global)
    : macro_param_base(macro_param_type::SYSTEM_TYPE, name, is_global)
    , data_(std::move(value))
{}

const C_t& system_variable::get_value(const std::vector<size_t>& offset) const { return get_data(offset)->get_value(); }

const C_t& system_variable::get_value(size_t idx) const { return macro_param_base::get_value(idx + 1); }

const C_t& system_variable::get_value() const { return macro_param_base::get_value(0); }

const macro_param_data_component* system_variable::get_data(const std::vector<size_t>& offset) const
{
    const macro_param_data_component* tmp = data_.get();

    for (size_t i = 0; i < offset.size(); ++i)
    {
        tmp = tmp->get_ith(offset[i] - (i == 0 ? 0 : 1));
    }

    return tmp;
}

A_t system_variable::number(std::vector<size_t> offset) const
{
    if (offset.empty())
        return (A_t)data_->number - 1;
    else
    {
        ++offset.front();
        return (A_t)macro_param_base::number(std::move(offset));
    }
}

A_t system_variable::count(std::vector<size_t> offset) const
{
    if (offset.empty())
        return (A_t)data_->get_ith(1)->get_value().size();

    const macro_param_data_component* tmp = real_data();
    for (size_t i = 0; i < offset.size(); ++i)
    {
        tmp = tmp->get_ith(offset[i] - (i == 0 ? 0 : 1));
    }
    return (A_t)tmp->get_value().size();
}

size_t system_variable::size(std::vector<size_t> offset) const
{
    const macro_param_data_component* tmp = real_data();

    for (size_t i = 0; i < offset.size(); ++i)
    {
        tmp = tmp->get_ith(offset[i] - (i == 0 ? 0 : 1));
    }

    return tmp->size();
}

const macro_param_data_component* system_variable::real_data() const { return &*data_; }
