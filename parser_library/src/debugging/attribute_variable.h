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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORD_SYM_ATTRIBUTE_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_ORD_SYM_ATTRIBUTE_VARIABLE_H

#include "context/ordinary_assembly/symbol.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::debugging {
// Implementation of variable interface that other variables use
// to show symbol attributes to the user.
class attribute_variable : public variable
{
public:
    attribute_variable(std::string name, std::string value);

    set_type type() const override;

    bool is_scalar() const override;

    std::vector<variable_ptr> values() const override;
    size_t size() const override;

protected:
    const std::string& get_string_value() const override;
    const std::string& get_string_name() const override;
};


} // namespace hlasm_plugin::parser_library::debugging


#endif // !HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_MACRO_PARAM_VARIABLE_H
