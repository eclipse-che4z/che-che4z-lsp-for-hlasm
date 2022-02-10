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

#ifndef CONTEXT_SYMBOL_H
#define CONTEXT_SYMBOL_H

#include "../source_context.h"
#include "address.h"
#include "location.h"
#include "symbol_attributes.h"
#include "symbol_value.h"

namespace hlasm_plugin::parser_library::context {

// class representing ordinary symbol
// the value and attributes fields have the same semantics as described in symbol_attributes
class symbol
{
public:
    symbol(id_index name,
        symbol_value value,
        symbol_attributes attributes,
        location symbol_location,
        processing_stack_t stack);

    const symbol_value& value() const;
    const symbol_attributes& attributes() const;

    symbol_value_kind kind() const;

    void set_value(symbol_value value);
    void set_length(symbol_attributes::len_attr value);
    void set_scale(symbol_attributes::scale_attr value);

    const id_index name;
    const location symbol_location;
    const processing_stack_t& proc_stack() const;

private:
    symbol_value value_;
    symbol_attributes attributes_;
    processing_stack_t stack_;
};

} // namespace hlasm_plugin::parser_library::context
#endif
