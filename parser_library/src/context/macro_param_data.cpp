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

#include "macro_param_data.h"

#include <stdexcept>

using namespace std;

namespace hlasm_plugin::parser_library::context {

macro_param_data_component::~macro_param_data_component() {}

macro_param_data_component::macro_param_data_component(size_t number)
    : number(number)
{}

const C_t& macro_param_data_single::get_value() const { return data_; }

const macro_data_shared_ptr macro_param_data_component::dummy(new macro_param_data_dummy());

macro_param_data_dummy::macro_param_data_dummy()
    : macro_param_data_component((size_t)0)
{}

const C_t& macro_param_data_dummy::get_value() const { return object_traits<C_t>::default_v(); }

const macro_param_data_component* macro_param_data_dummy::get_ith(size_t) const { return this; }

size_t macro_param_data_dummy::size() const { return 0; }

const macro_param_data_component* macro_param_data_single::get_ith(size_t idx) const
{
    if (idx == 0)
        return this;
    return macro_param_data_component::dummy.get();
}

size_t macro_param_data_single::size() const { return 0; }

macro_param_data_single::macro_param_data_single(C_t value)
    : macro_param_data_component(value.empty() ? 0 : 1)
    , data_(std::move(value))
{}


const C_t& macro_param_data_composite::get_value() const
{
    if (value_.empty())
    {
        value_.append("(");
        for (size_t i = 0; i < data_.size(); ++i)
        {
            value_.append(data_[i]->get_value());
            if (i != data_.size() - 1)
                value_.append(",");
        }
        value_.append(")");
    }
    return value_;
}

const macro_param_data_component* macro_param_data_composite::get_ith(size_t idx) const
{
    if (idx < data_.size())
        return data_[idx].get();
    return macro_param_data_component::dummy.get();
}

size_t macro_param_data_composite::size() const { return data_.size(); }


macro_param_data_composite::macro_param_data_composite(std::vector<macro_data_ptr> value)
    : macro_param_data_component(value.size())
    , data_(move(value))
{}

macro_param_data_single_dynamic::macro_param_data_single_dynamic()
    : macro_param_data_single("")
{}

} // namespace hlasm_plugin::parser_library::context
