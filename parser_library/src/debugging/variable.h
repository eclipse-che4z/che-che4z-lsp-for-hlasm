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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H

#include <memory>
#include <optional>

#include "context/variables/variable.h"
#include "protocol.h"

namespace hlasm_plugin::parser_library::debugging {
class variable;

using variable_ptr = std::unique_ptr<variable>;

// Interface that represents a variable to be shown to the user
// through DAP.
class variable
{
public:
    virtual const std::string& get_name() const = 0;
    virtual const std::string& get_value() const = 0;

    virtual set_type type() const = 0;

    virtual bool is_scalar() const = 0;

    virtual std::vector<variable_ptr> values() const = 0;

    var_reference_t var_reference = 0;

    virtual ~variable() = default;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H
