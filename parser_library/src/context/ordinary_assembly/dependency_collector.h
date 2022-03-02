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

#ifndef CONTEXT_DEPENDENCY_COLLECTOR_H
#define CONTEXT_DEPENDENCY_COLLECTOR_H

#include <optional>
#include <set>

#include "address.h"
#include "symbol_attributes.h"

namespace hlasm_plugin::parser_library::context {

// helper structure that holds dependencies throughout whole process of getting dependencies
struct dependency_collector
{
    using attr_ref = std::pair<data_attr_kind, id_index>;
    struct error
    {};

    // errorous holder
    bool has_error = false;
    // dependent address
    std::optional<address> unresolved_address;
    // dependent symbol
    std::set<id_index> undefined_symbols;
    // dependent symbol dependencies
    std::set<attr_ref> undefined_attr_refs;
    // unresolved spaces that must be resolved due to * or / operator
    // nonempty when address without base but with spaces is multiplied or divided
    std::set<space_ptr> unresolved_spaces;

    dependency_collector();
    dependency_collector(error);
    dependency_collector(id_index undefined_symbol);
    dependency_collector(address unresolved_address);
    dependency_collector(attr_ref attribute_reference);

    dependency_collector& operator+=(const dependency_collector& holder);
    dependency_collector& operator-=(const dependency_collector& holder);
    dependency_collector& operator*=(const dependency_collector& holder);
    dependency_collector& operator/=(const dependency_collector& holder);

    dependency_collector& merge(const dependency_collector& dc);

    bool is_address() const;

    bool contains_dependencies() const;

private:
    bool merge_undef(const dependency_collector& holder);

    dependency_collector& add_sub(const dependency_collector& holder, bool add);

    dependency_collector& div_mul(const dependency_collector& holder);

    static void adjust_address(address& addr);
};

} // namespace hlasm_plugin::parser_library::context
#endif