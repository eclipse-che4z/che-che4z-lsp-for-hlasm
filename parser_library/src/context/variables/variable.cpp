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

namespace hlasm_plugin::parser_library::context {

set_symbol_base* variable_symbol::access_set_symbol_base()
{
    return var_kind == variable_kind::SET_VAR_KIND ? static_cast<set_symbol_base*>(this) : nullptr;
}

const set_symbol_base* variable_symbol::access_set_symbol_base() const
{
    return var_kind == variable_kind::SET_VAR_KIND ? static_cast<const set_symbol_base*>(this) : nullptr;
}

macro_param_base* variable_symbol::access_macro_param_base()
{
    return var_kind == variable_kind::MACRO_VAR_KIND ? static_cast<macro_param_base*>(this) : nullptr;
}

const macro_param_base* variable_symbol::access_macro_param_base() const
{
    return var_kind == variable_kind::MACRO_VAR_KIND ? static_cast<const macro_param_base*>(this) : nullptr;
}

variable_symbol::variable_symbol(variable_kind var_kind, id_index name)
    : id(name)
    , var_kind(var_kind)
{}

} // namespace hlasm_plugin::parser_library::context
