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

namespace hlasm_plugin::parser_library::context {

position from_stack(processing_stack_t stack) noexcept
{
    if (stack.empty())
        return position();
    auto p = stack.frame().pos;
    p.column = 0;
    return p;
}

symbol::symbol(id_index name,
    symbol_value value,
    symbol_attributes attributes,
    processing_stack_t stack,
    std::optional<position> pos) noexcept
    : name_(name)
    , value_(std::move(value))
    , attributes_(attributes)
    , pos_(std::move(pos).value_or(from_stack(stack)))
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

location symbol::symbol_location() const
{
    if (stack_.empty())
        return location();

    auto sym_loc = stack_.frame().get_location();
    sym_loc.pos = pos_;
    return sym_loc;
}

const processing_stack_t& symbol::proc_stack() const { return stack_; }

} // namespace hlasm_plugin::parser_library::context
