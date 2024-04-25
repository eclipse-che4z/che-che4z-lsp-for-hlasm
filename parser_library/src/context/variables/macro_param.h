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

#ifndef CONTEXT_MACRO_PARAM_H
#define CONTEXT_MACRO_PARAM_H

#include "context/macro_param_data.h"
#include "variable.h"

namespace hlasm_plugin::parser_library::context {

class keyword_param;
class positional_param;
class system_variable;

// class wrapping macro parameters data
class macro_param_base : public variable_symbol
{
public:
    // returns type of macro parameter
    const macro_param_type param_type;

    const keyword_param* access_keyword_param() const;
    const positional_param* access_positional_param() const;
    const system_variable* access_system_variable() const;

    // gets value of data where parameter is list of nested data offsets
    virtual C_t get_value(std::span<const A_t> offset) const;
    // gets value of data where parameter is offset to data field
    virtual C_t get_value(A_t idx) const;
    // gets value of whole macro parameter
    virtual C_t get_value() const;

    // gets param struct
    virtual const macro_param_data_component* get_data(std::span<const A_t> offset) const;

    // N' attribute of the symbol
    A_t number(std::span<const A_t> offset) const override;
    // K' attribute of the symbol
    A_t count(std::span<const A_t> offset) const override;

    bool can_read(
        std::span<const A_t> subscript, range symbol_range, diagnostic_consumer_t<diagnostic_op>& diags) const override;

    virtual std::optional<std::pair<A_t, A_t>> index_range(std::span<const A_t> offset) const;

protected:
    macro_param_base(macro_param_type param_type, id_index name);
    virtual const macro_param_data_component* real_data() const = 0;
};

// represent macro param with stated position and name, positional param
class keyword_param : public macro_param_base
{
    const macro_data_ptr assigned_data_;

public:
    keyword_param(id_index name, macro_data_shared_ptr default_value, macro_data_ptr assigned_value);

    // default macro keyword parameter data
    const macro_data_shared_ptr default_data;

protected:
    const macro_param_data_component* real_data() const override;
};

// represents macro param with default value, name and no position, keyword param
class positional_param : public macro_param_base
{
    const macro_param_data_component& data_;

public:
    const size_t position;

    positional_param(id_index name, size_t position, const macro_param_data_component& assigned_value);

protected:
    const macro_param_data_component* real_data() const override;
};

} // namespace hlasm_plugin::parser_library::context

#endif
