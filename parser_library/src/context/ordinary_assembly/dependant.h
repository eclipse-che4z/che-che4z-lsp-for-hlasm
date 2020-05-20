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

#ifndef SEMANTICS_SYMBOL_DEPENDANT_H
#define SEMANTICS_SYMBOL_DEPENDANT_H

#include <variant>

#include "address.h"
#include "symbol_attributes.h"

namespace hlasm_plugin {
namespace parser_library {
namespace context {

// structure representing symbol attribute reference
struct attr_ref
{
    data_attr_kind attribute;
    id_index symbol_id;

    bool operator==(const attr_ref& oth) const;
};

enum class dependant_kind
{
    SYMBOL = 0,
    SYMBOL_ATTR = 1,
    SPACE = 2
};

// structure representing objects with dependencies
struct dependant
{
    using value_t = std::variant<id_index, attr_ref, space_ptr>;

    dependant(id_index symbol_id);
    dependant(attr_ref attribute_reference);
    dependant(space_ptr space_id);

    bool operator==(const dependant& oth) const;
    dependant_kind kind() const;

    value_t value;
};

} // namespace context
} // namespace parser_library
} // namespace hlasm_plugin

namespace std {
template<>
struct hash<hlasm_plugin::parser_library::context::attr_ref>
{
    std::size_t operator()(const hlasm_plugin::parser_library::context::attr_ref& k) const
    {
        return std::hash<hlasm_plugin::parser_library::context::id_index>()(k.symbol_id) + (size_t)k.attribute;
    }
};

template<>
struct hash<hlasm_plugin::parser_library::context::dependant>
{
    std::size_t operator()(const hlasm_plugin::parser_library::context::dependant& k) const
    {
        return hash<hlasm_plugin::parser_library::context::dependant::value_t>()(k.value);
    }
};
} // namespace std

#endif
