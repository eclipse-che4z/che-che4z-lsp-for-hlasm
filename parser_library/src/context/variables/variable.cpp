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

#include "variable.h"

#include "macro_param.h"
#include "set_symbol.h"

using namespace std;

namespace hlasm_plugin::parser_library::context {

set_symbol_base* variable_symbol::access_set_symbol_base() { return dynamic_cast<set_symbol_base*>(this); }

const set_symbol_base* variable_symbol::access_set_symbol_base() const
{
    return dynamic_cast<const set_symbol_base*>(this);
}

macro_param_base* variable_symbol::access_macro_param_base() { return dynamic_cast<macro_param_base*>(this); }

const macro_param_base* variable_symbol::access_macro_param_base() const
{
    return dynamic_cast<const macro_param_base*>(this);
}

variable_symbol::variable_symbol(variable_kind var_kind, id_index name, bool is_global)
    : id(name)
    , is_global(is_global)
    , var_kind(var_kind)
{}

} // namespace hlasm_plugin::parser_library::context
