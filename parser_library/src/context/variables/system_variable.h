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

#ifndef CONTEXT_SYSTEM_VARIABLE_H
#define CONTEXT_SYSTEM_VARIABLE_H

#include "macro_param.h"

namespace hlasm_plugin::parser_library::context {

// base for variable symbols
class system_variable : public macro_param_base
{
    const macro_data_ptr data_;

public:
    system_variable(id_index name, macro_data_ptr value);

    // gets value of data where parameter is list of nested data offsets
    C_t get_value(std::span<const A_t> offset) const override;
    // gets value of data where parameter is offset to data field
    C_t get_value(A_t idx) const override;
    // gets value of whole macro parameter
    C_t get_value() const override;

    // gets param struct
    const macro_param_data_component* get_data(std::span<const A_t> offset) const override;

    // N' attribute of the symbol
    A_t number(std::span<const A_t> offset) const override;
    // K' attribute of the symbol
    A_t count(std::span<const A_t> offset) const override;

    std::optional<std::pair<A_t, A_t>> index_range(std::span<const A_t> offset) const override;

protected:
    const macro_param_data_component* real_data() const override;
};

// SYSMAC extras
class system_variable_sysmac final : public system_variable
{
public:
    using system_variable::system_variable;

    // SYSMAC special behavior
    C_t get_value(std::span<const A_t> offset) const override;
    C_t get_value(A_t idx) const override;
    C_t get_value() const override;
    const macro_param_data_component* get_data(std::span<const A_t> offset) const override;
    bool can_read(
        std::span<const A_t> subscript, range symbol_range, diagnostic_consumer_t<diagnostic_op>& diags) const override;
};

// SYSLIST extras
class system_variable_syslist final : public system_variable
{
public:
    using system_variable::system_variable;

    // SYSLIST special behavior
    const macro_param_data_component* get_data(std::span<const A_t> offset) const override;
    bool can_read(
        std::span<const A_t> subscript, range symbol_range, diagnostic_consumer_t<diagnostic_op>& diags) const override;
};

} // namespace hlasm_plugin::parser_library::context

#endif
