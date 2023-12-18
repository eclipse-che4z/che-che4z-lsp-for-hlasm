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

#include <cassert>
#include <limits>

namespace hlasm_plugin::parser_library::context {

macro_param_data_component::~macro_param_data_component() = default;

macro_param_data_component::macro_param_data_component(A_t number)
    : number_of_components(number)
{}

C_t macro_param_data_single::get_value() const { return data_; }

const macro_data_shared_ptr macro_param_data_component::dummy(new macro_param_data_dummy());

macro_param_data_dummy::macro_param_data_dummy()
    : macro_param_data_component(0)
{}

C_t macro_param_data_dummy::get_value() const { return object_traits<C_t>::default_v(); }

const macro_param_data_component* macro_param_data_dummy::get_ith(A_t) const { return this; }

std::optional<std::pair<A_t, A_t>> macro_param_data_dummy::index_range() const { return std::nullopt; }

const macro_param_data_component* macro_param_data_single::get_ith(A_t idx) const
{
    if (idx == 1)
        return this;
    return macro_param_data_component::dummy.get();
}

std::optional<std::pair<A_t, A_t>> macro_param_data_single::index_range() const { return std::nullopt; }

macro_param_data_single::macro_param_data_single(C_t value)
    : macro_param_data_component(value.empty() ? 0 : 1)
    , data_(std::move(value))
{}

C_t macro_param_data_composite::get_value() const { return value_; }

const macro_param_data_component* macro_param_data_composite::get_ith(A_t idx) const
{
    if (0 < idx && idx <= data_.size())
        return data_[idx - 1].get();
    return macro_param_data_component::dummy.get();
}

std::optional<std::pair<A_t, A_t>> macro_param_data_composite::index_range() const
{
    return std::pair<A_t, A_t>(1, (A_t)data_.size());
}


macro_param_data_composite::macro_param_data_composite(std::vector<macro_data_ptr> value)
    : macro_param_data_component(!value.empty() ? (A_t)value.size() : 1)
    , data_(!value.empty() ? std::move(value) : [] {
        std::vector<context::macro_data_ptr> vec;
        vec.push_back(std::make_unique<context::macro_param_data_dummy>());
        return vec;
    }())
{
    assert(data_.size() <= std::numeric_limits<A_t>::max());
    value_.append("(");
    for (size_t i = 0; i < data_.size(); ++i)
    {
        value_.append(data_[i]->get_value());
        if (i != data_.size() - 1)
            value_.append(",");
    }
    value_.append(")");
}

C_t macro_param_data_zero_based::get_value() const { return value_; }

const macro_param_data_component* macro_param_data_zero_based::get_ith(A_t idx) const
{
    if (0 <= idx && idx < data_.size())
        return data_[idx].get();
    return macro_param_data_component::dummy.get();
}

std::optional<std::pair<A_t, A_t>> macro_param_data_zero_based::index_range() const
{
    return std::pair<A_t, A_t>(0, (A_t)data_.size() - 1);
}


macro_param_data_zero_based::macro_param_data_zero_based(std::vector<macro_data_ptr> value)
    : macro_param_data_component(!value.empty() ? (A_t)value.size() : 1)
    , data_(!value.empty() ? std::move(value) : [] {
        std::vector<context::macro_data_ptr> vec;
        vec.push_back(std::make_unique<context::macro_param_data_dummy>());
        return vec;
    }())
{
    assert(data_.size() <= std::numeric_limits<A_t>::max());
    value_.append("(");
    for (size_t i = 0; i < data_.size(); ++i)
    {
        value_.append(data_[i]->get_value());
        if (i != data_.size() - 1)
            value_.append(",");
    }
    value_.append(")");
}

C_t macro_param_data_single_dynamic::get_value() const { return get_dynamic_value(); }

macro_param_data_single_dynamic::macro_param_data_single_dynamic()
    : macro_param_data_single("")
{}

} // namespace hlasm_plugin::parser_library::context
