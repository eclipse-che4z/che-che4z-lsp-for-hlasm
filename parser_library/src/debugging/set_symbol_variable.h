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

#ifndef HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_SET_SYMBOL_VARIABLE_H
#define HLASMPLUGIN_PARSERLIBRARY_DEBUGGING_SET_SYMBOL_VARIABLE_H

#include <optional>

#include "context/variables/set_symbol.h"
#include "variable.h"


namespace hlasm_plugin::parser_library::debugging {
// Implementation of variable interface that adapts set symbol
// representation from context to DAP variable.
class set_symbol_variable : public variable
{
public:
    set_symbol_variable(const context::set_symbol_base& set_sym, int index);
    set_symbol_variable(const context::set_symbol_base& set_sym);

    const std::string& get_name() const override;
    const std::string& get_value() const override;

    set_type type() const override;

    bool is_scalar() const override;

    std::vector<variable_ptr> values() const override;
    size_t size() const override;

private:
    template<typename T>
    T get_value(const std::optional<int>& index) const;

    std::string get_string_value(const std::optional<int>& index) const;
    std::string get_string_array_value() const;

    const context::set_symbol_base& set_symbol_;
    const std::optional<int> index_;

    const std::string name_;
    std::string value_;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
