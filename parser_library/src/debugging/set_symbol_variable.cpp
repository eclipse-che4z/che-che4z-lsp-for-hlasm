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

#include "set_symbol_variable.h"

#include <cassert>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;

void set_symbol_variable::fill_string_value()
{
    if (!is_scalar())
        value_.emplace("");
    else if (type() == set_type::A_TYPE)
        value_.emplace(std::to_string(get_value<context::A_t>()));
    else if (type() == set_type::B_TYPE)
        value_.emplace(get_value<context::B_t>() ? "TRUE" : "FALSE");
}


set_symbol_variable::set_symbol_variable(const context::set_symbol_base& set_sym, int index)
    : set_symbol_(set_sym)
    , index_(index)
{
    name_.emplace(std::to_string(index + 1));
    fill_string_value();
}

set_symbol_variable::set_symbol_variable(const context::set_symbol_base& set_sym)
    : set_symbol_(set_sym)
    , index_()
{
    fill_string_value();
}

const std::string& set_symbol_variable::get_string_value() const { return get_value<std::string>(); }


set_type set_symbol_variable::type() const { return (set_type)set_symbol_.type; }

const std::string& set_symbol_variable::get_string_name() const { return *set_symbol_.id; }

bool set_symbol_variable::is_scalar() const
{
    if (index_)
        // set symbols may only have one level of nesting
        return true;
    else
        return set_symbol_.is_scalar;
}

std::vector<variable_ptr> set_symbol_variable::values() const
{
    std::vector<std::unique_ptr<debugging::variable>> vals;

    auto keys = set_symbol_.keys();
    for (size_t i = 0; i < keys.size(); ++i)
        vals.push_back(std::make_unique<set_symbol_variable>(set_symbol_, (int)keys[i]));

    return vals;
}

size_t set_symbol_variable::size() const { return set_symbol_.size(); }

template<typename T>
inline const T& set_symbol_variable::get_value() const
{
    if (set_symbol_.is_scalar)
        return set_symbol_.access_set_symbol<T>()->get_value();
    else if (index_)
        return set_symbol_.access_set_symbol<T>()->get_value(*index_);
    else
        return set_symbol_.access_set_symbol<T>()->get_value();
}
