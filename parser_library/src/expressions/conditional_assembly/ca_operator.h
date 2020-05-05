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

#include "ca_expresssion.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_unary_operator : public ca_expression
{
public:
    const ca_expr_ptr expr;
};

class ca_binary_operator : public ca_expression
{
public:
    const ca_expr_ptr left_expr, right_expr;
};



} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
