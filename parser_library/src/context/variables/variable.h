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

#ifndef CONTEXT_VARIABLE_H
#define CONTEXT_VARIABLE_H

#include <span>

#include "context/common_types.h"
#include "context/id_index.h"

namespace hlasm_plugin::parser_library {
struct diagnostic_op;
template<typename T>
class diagnostic_consumer_t;
struct range;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::context {

class set_symbol_base;
class macro_param_base;
class variable_symbol;

// base for variable symbols
class variable_symbol
{
public:
    // name of the symbol
    const id_index id;

    // returns kind of variable symbol
    const variable_kind var_kind;

    // casts this to set_symbol_base
    set_symbol_base* access_set_symbol_base();
    const set_symbol_base* access_set_symbol_base() const;
    // casts this to macro_param
    macro_param_base* access_macro_param_base();
    const macro_param_base* access_macro_param_base() const;

    // N' attribute of the symbol
    virtual A_t number(std::span<const A_t> offset) const = 0;
    // K' attribute of the symbol
    virtual A_t count(std::span<const A_t> offset) const = 0;

    virtual bool can_read(
        std::span<const A_t> subscript, range symbol_range, diagnostic_consumer_t<diagnostic_op>& diags) const = 0;

    virtual ~variable_symbol() = default;

protected:
    variable_symbol(variable_kind var_kind, id_index name);
};

} // namespace hlasm_plugin::parser_library::context

#endif
