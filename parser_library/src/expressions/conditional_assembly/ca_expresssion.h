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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPRESSION_H

#include <memory>

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_expression;
using ca_expr_ptr = std::unique_ptr<ca_expresssion>;

class ca_expression
{};



} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin

#endif
