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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_OPERATOR_H

#include <vector>

#include "ca_expresssion.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {


class ca_expr_list : public ca_expression
{
    using ca_expr_list_t = std::vector<const ca_expr_ptr>;

public:
    const ca_expr_list_t expr_list;
};

class ca_string : public ca_expression
{};

class variable_symbol : public ca_expression
{};

class ca_constant : public ca_expression
{};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
