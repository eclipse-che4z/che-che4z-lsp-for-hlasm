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
#include <cassert>
#include <optional>
#include <vector>

#include "address.h"
#include "dependant.h"
#include "symbol_attributes.h"

namespace hlasm_plugin::parser_library::context {

struct symbolic_reference
{
    id_index name;
    unsigned flags = 0;

    explicit symbolic_reference() = default;
    explicit symbolic_reference(id_index name)
        : name(name)
    {
        set();
    }
    explicit symbolic_reference(id_index name, data_attr_kind attr)
        : name(name)
    {
        set(attr);
    }

    void set() { flags |= 1u; }
    bool get() const { return flags & 1u; }
    void set(data_attr_kind attr)
    {
        assert(attr != data_attr_kind::UNKNOWN);
        flags |= 1u << static_cast<int>(attr);
    }
    bool get(data_attr_kind attr) const
    {
        assert(attr != data_attr_kind::UNKNOWN);
        return flags & 1u << static_cast<int>(attr);
    }

    bool has_only(data_attr_kind attr) const
    {
        assert(attr != data_attr_kind::UNKNOWN);
        return (flags & 1u << static_cast<int>(attr)) == 1u << static_cast<int>(attr);
    }
};

// helper structure that holds dependencies throughout whole process of getting dependencies
struct dependency_collector
{
    struct error
    {};

    // errorous holder
    bool has_error = false;

    // dependent address
    std::optional<address> unresolved_address;

    // symbolic dependencies
    std::vector<symbolic_reference> undefined_symbolics;

    // unresolved spaces that must be resolved due to * or / operator
    // nonempty when address without base but with spaces is multiplied or divided
    std::vector<space_ptr> unresolved_spaces;

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

    void collect_unique_symbolic_dependencies(std::vector<context::id_index>& missing_symbols) const;

private:
    bool merge_undef(const dependency_collector& holder);

    void add_sub(const dependency_collector& holder, bool add);

    void div_mul(const dependency_collector& holder);
};

} // namespace hlasm_plugin::parser_library::context
#endif
