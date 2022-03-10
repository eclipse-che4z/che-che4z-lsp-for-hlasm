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

#include "macro_param_variable.h"

#include <cstddef>
#include <stdexcept>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;

macro_param_variable::macro_param_variable(const context::macro_param_base& param, std::vector<size_t> index)
    : variable(name_, value_)
    , macro_param_(param)
    , index_(std::move(index))
{
    if (!index_.empty())
        name_ = std::to_string(index_.back());
    else
        name_ = "&" + *macro_param_.id;

    value_ = macro_param_.get_value(index_);
}

set_type macro_param_variable::type() const { return set_type::C_TYPE; }

bool macro_param_variable::is_scalar() const { return macro_param_.size(index_) == 0; }

std::vector<variable_ptr> macro_param_variable::values() const
{
    std::vector<std::unique_ptr<variable>> vals;

    std::vector<size_t> child_index = index_;
    child_index.push_back(0);

    if (macro_param_.access_system_variable() && child_index.size() == 1)
    {
        for (size_t i = 0; i < size(); ++i)
        {
            child_index.back() = i;
            vals.push_back(std::make_unique<macro_param_variable>(macro_param_, child_index));
        }
    }
    else
    {
        for (size_t i = 1; i <= size(); ++i)
        {
            child_index.back() = i;
            vals.push_back(std::make_unique<macro_param_variable>(macro_param_, child_index));
        }
    }
    return vals;
}

size_t macro_param_variable::size() const { return macro_param_.size(index_); }
