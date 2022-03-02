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

#include "dependency_collector.h"

#include <algorithm>

using namespace hlasm_plugin::parser_library::context;

dependency_collector::dependency_collector() = default;

dependency_collector::dependency_collector(error)
    : has_error(true)
{}

dependency_collector::dependency_collector(id_index undefined_symbol) { undefined_symbols.insert(undefined_symbol); }

dependency_collector::dependency_collector(address u_a)
    : unresolved_address(std::move(u_a))
{
    unresolved_address->normalize();
}

dependency_collector::dependency_collector(attr_ref attribute_reference)
{
    undefined_attr_refs.insert(std::move(attribute_reference));
}

dependency_collector& dependency_collector::operator+=(const dependency_collector& holder)
{
    if (!merge_undef(holder))
        add_sub(holder, true);

    return *this;
}

dependency_collector& dependency_collector::operator-=(const dependency_collector& holder)
{
    if (!merge_undef(holder))
        add_sub(holder, false);

    return *this;
}

dependency_collector& dependency_collector::operator*=(const dependency_collector& holder)
{
    if (!merge_undef(holder))
        div_mul(holder);

    return *this;
}

dependency_collector& dependency_collector::operator/=(const dependency_collector& holder)
{
    if (!merge_undef(holder))
        div_mul(holder);

    return *this;
}

dependency_collector& hlasm_plugin::parser_library::context::dependency_collector::merge(const dependency_collector& dc)
{
    merge_undef(dc);

    if (unresolved_address)
    {
        for (const auto& [sp, c] : unresolved_address->spaces())
            unresolved_spaces.insert(sp);
        unresolved_address.reset();
    }

    if (dc.unresolved_address)
    {
        for (const auto& [sp, c] : dc.unresolved_address->spaces())
            unresolved_spaces.insert(sp);
    }

    return *this;
}

bool dependency_collector::is_address() const
{
    return undefined_symbols.empty() && unresolved_address && !unresolved_address.value().bases().empty();
}

bool dependency_collector::contains_dependencies() const
{
    return !undefined_symbols.empty() || !undefined_attr_refs.empty() || !unresolved_spaces.empty()
        || (unresolved_address && unresolved_address->has_unresolved_space());
}

bool dependency_collector::merge_undef(const dependency_collector& holder)
{
    has_error |= holder.has_error;

    undefined_symbols.insert(holder.undefined_symbols.begin(), holder.undefined_symbols.end());

    undefined_attr_refs.insert(holder.undefined_attr_refs.begin(), holder.undefined_attr_refs.end());

    unresolved_spaces.insert(holder.unresolved_spaces.begin(), holder.unresolved_spaces.end());

    return has_error || !undefined_symbols.empty();
}

void dependency_collector::add_sub(const dependency_collector& holder, bool add)
{
    if (unresolved_address && holder.unresolved_address)
    {
        if (add)
            unresolved_address = *unresolved_address + (*holder.unresolved_address);
        else
            unresolved_address = *unresolved_address - (*holder.unresolved_address);
        adjust_address(*unresolved_address);
    }
    else if (!unresolved_address && holder.unresolved_address)
    {
        if (add)
            unresolved_address = *holder.unresolved_address;
        else
            unresolved_address = -*holder.unresolved_address;
    }
}

void dependency_collector::div_mul(const dependency_collector& holder)
{
    if (is_address() || holder.is_address())
        has_error = true;
    else
    {
        if (unresolved_address)
            for (const auto& [sp, c] : unresolved_address->spaces())
                unresolved_spaces.insert(sp);
        if (holder.unresolved_address)
            for (const auto& [sp, c] : holder.unresolved_address->spaces())
                unresolved_spaces.insert(sp);
    }
}

void dependency_collector::adjust_address(address& addr)
{
    auto known_spaces = std::partition(addr.spaces().begin(), addr.spaces().end(), [](auto& entry) {
        return entry.first->kind == context::space_kind::LOCTR_UNKNOWN;
    });

    if (known_spaces != addr.spaces().begin())
        addr.spaces().erase(known_spaces, addr.spaces().end());
}
