/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef CONTEXT_SYMBOL_VALUE_H
#define CONTEXT_SYMBOL_VALUE_H

#include <variant>

#include "address.h"

namespace hlasm_plugin::parser_library::context {

// defines kind of symbol value, absolute or relocatable or undefined
enum class symbol_value_kind
{
    UNDEF = 0,
    ABS = 1,
    RELOC = 2
};

// structure holding value of a symbol
struct symbol_value
{
    using abs_value_t = int32_t;
    using reloc_value_t = address;

    symbol_value(abs_value_t value);
    symbol_value(reloc_value_t value);
    symbol_value();

    symbol_value operator+(const symbol_value& value) const;
    symbol_value operator-(const symbol_value& value) const;
    symbol_value operator*(const symbol_value& value) const;
    symbol_value operator/(const symbol_value& value) const;
    symbol_value operator-() const;

    const abs_value_t& get_abs() const;
    const reloc_value_t& get_reloc() const;

    symbol_value_kind value_kind() const;

    symbol_value ignore_qualification() const;

private:
    std::variant<std::monostate, abs_value_t, reloc_value_t> value_;
};

} // namespace hlasm_plugin::parser_library::context
#endif
