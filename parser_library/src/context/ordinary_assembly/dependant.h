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

#ifndef CONTEXT_DEPENDANT_H
#define CONTEXT_DEPENDANT_H

#include <compare>
#include <variant>

#include "address.h"
#include "symbol_attributes.h"

namespace hlasm_plugin::parser_library::context {

// structure representing symbol attribute reference
struct attr_ref
{
    data_attr_kind attribute;
    id_index symbol_id;

    auto operator<=>(const attr_ref&) const = default;
};

// structure representing objects with dependencies
using dependant = std::variant<id_index, attr_ref, space_ptr>;

} // namespace hlasm_plugin::parser_library::context

namespace std {
template<>
struct hash<hlasm_plugin::parser_library::context::attr_ref>
{
    std::size_t operator()(const hlasm_plugin::parser_library::context::attr_ref& k) const
    {
        return std::hash<hlasm_plugin::parser_library::context::id_index>()(k.symbol_id) + (size_t)k.attribute;
    }
};

} // namespace std

#endif
