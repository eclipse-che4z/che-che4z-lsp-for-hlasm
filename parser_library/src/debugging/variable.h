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
    const std::string& get_value() const;
    const std::string& get_name() const;

    virtual set_type type() const = 0;

    virtual bool is_scalar() const = 0;

    virtual std::vector<variable_ptr> values() const = 0;
    virtual size_t size() const = 0;

    var_reference_t var_reference = 0;

    virtual ~variable() = default;

protected:
    virtual std::string get_string_value() const = 0;
    virtual const std::string& get_string_name() const = 0;

    std::optional<std::string> name_;
    std::optional<std::string> value_;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_VARIABLE_H
