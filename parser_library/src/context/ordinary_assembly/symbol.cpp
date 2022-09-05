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

#include "symbol.h"

#include <assert.h>
#include <stdexcept>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

symbol::symbol(
    id_index name, symbol_value value, symbol_attributes attributes, location symbol_location, processing_stack_t stack)
    : name_(name)
    , symbol_location_(std::move(symbol_location))
    , value_(std::move(value))
    , attributes_(attributes)
    , stack_(std::move(stack))
{}

void symbol::set_value(symbol_value value)
{
    if (value.value_kind() == symbol_value_kind::UNDEF)
        throw std::runtime_error("can not assign undefined value");

    if (kind() != symbol_value_kind::UNDEF)
        throw std::runtime_error("can not assign value to already defined symbol");

    value_ = std::move(value);
}

void symbol::set_length(symbol_attributes::len_attr value) { attributes_.length(value); }

void symbol::set_scale(symbol_attributes::scale_attr value) { attributes_.scale(value); }

const processing_stack_t& symbol::proc_stack() const { return stack_; }
