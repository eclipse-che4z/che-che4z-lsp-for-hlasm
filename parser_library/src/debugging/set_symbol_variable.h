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

    set_type type() const override;

    bool is_scalar() const override;

    virtual std::vector<variable_ptr> values() const override;
    size_t size() const override;

protected:
    virtual const std::string& get_string_value() const override;
    virtual const std::string& get_string_name() const override;

private:
    template<typename T>
    const T& get_value() const;

    void fill_string_value();

    const context::set_symbol_base& set_symbol_;
    const std::optional<int> index_;
};

} // namespace hlasm_plugin::parser_library::debugging

#endif
