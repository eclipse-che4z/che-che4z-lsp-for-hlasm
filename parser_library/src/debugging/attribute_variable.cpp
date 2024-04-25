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

#include "debug_types.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging {

variable generate_attribute_variable(std::string name, std::string value)
{
    return variable {
        .name = std::move(name),
        .value = std::move(value),
    };
}

} // namespace hlasm_plugin::parser_library::debugging
