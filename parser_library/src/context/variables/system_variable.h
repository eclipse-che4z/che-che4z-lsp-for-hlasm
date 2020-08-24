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

namespace hlasm_plugin {
namespace parser_library {
namespace context {

class system_variable;
using sys_sym_ptr = std::shared_ptr<system_variable>;

// base for variable symbols
class system_variable : public macro_param_base
{
    const macro_data_ptr data_;

public:
    system_variable(id_index name, macro_data_ptr value, bool is_global);

    // gets value of data where parameter is list of nested data offsets
    virtual const C_t& get_value(const std::vector<size_t>& offset) const override;
    // gets value of data where parameter is offset to data field
    virtual const C_t& get_value(size_t idx) const override;
    // gets value of whole macro parameter
    virtual const C_t& get_value() const override;

    // gets param struct
    virtual const macro_param_data_component* get_data(const std::vector<size_t>& offset) const override;

    // N' attribute of the symbol
    virtual A_t number(std::vector<size_t> offset) const override;
    // K' attribute of the symbol
    virtual A_t count(std::vector<size_t> offset) const override;

    virtual size_t size(std::vector<size_t> offset) const override;

protected:
    virtual const macro_param_data_component* real_data() const override;
};

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin

#endif
