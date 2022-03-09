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

#include "set_symbol.h"

#include "diagnostic.h"
#include "diagnostic_consumer.h"

using namespace hlasm_plugin::parser_library::context;

bool set_symbol_base::can_read(
    const std::vector<context::A_t>& subscript, range symbol_range, diagnostic_consumer<diagnostic_op>& diags) const
{
    if (subscript.size() > 1)
    {
        diags.add_diagnostic(
            diagnostic_op::error_E020("variable symbol subscript", symbol_range)); // error - too many operands
        return false;
    }

    if ((is_scalar && subscript.size() == 1) || (!is_scalar && subscript.empty()))
    {
        diags.add_diagnostic(
            diagnostic_op::error_E013("subscript error", symbol_range)); // error - inconsistent format of subcript
        return false;
    }

    if (!is_scalar && (subscript.front() < 1))
    {
        diags.add_diagnostic(diagnostic_op::error_E012(
            "subscript value has to be 1 or more", symbol_range)); // error - subscript is less than 1
        return false;
    }

    return true;
}

set_symbol_base::set_symbol_base(id_index name, bool is_scalar, bool is_global, SET_t_enum type)
    : variable_symbol(variable_kind::SET_VAR_KIND, name, is_global)
    , is_scalar(is_scalar)
    , type(type)
{}