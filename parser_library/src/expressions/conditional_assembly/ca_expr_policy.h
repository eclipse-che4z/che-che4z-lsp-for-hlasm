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

#ifndef HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_POLICY_H
#define HLASMPLUGIN_PARSERLIBRARY_CA_EXPR_POLICY_H

#include "context/id_storage.h"

namespace hlasm_plugin {
namespace parser_library {
namespace expressions {

class ca_arithmetic_policy
{
public:
    static bool is_unary(context::id_index symbol);

    static bool is_binary(context::id_index symbol);

    static bool is_operator(context::id_index symbol);

    static bool get_priority(context::id_index symbol);
};

} // namespace expressions
} // namespace parser_library
} // namespace hlasm_plugin


#endif
