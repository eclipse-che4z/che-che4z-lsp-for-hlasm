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

using namespace hlasm_plugin::parser_library::context;

set_symbol_base::set_symbol_base(id_index name, bool is_scalar, bool is_global, SET_t_enum type)
    : variable_symbol(variable_kind::SET_VAR_KIND, name, is_global)
    , is_scalar(is_scalar)
    , type(type)
{ }