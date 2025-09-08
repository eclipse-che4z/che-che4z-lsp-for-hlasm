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

#include "address_resolver.h"

#include <algorithm>
#include <assert.h>

#include "symbol_value.h"

using namespace hlasm_plugin::parser_library::context;

address_resolver::address_resolver(address dependency_address_, size_t boundary)
    : dependency_address(std::move(dependency_address_))
    , boundary(boundary)
{}

dependency_collector address_resolver::get_dependencies(dependency_solver&) const
{
    return extract_dep_address(dependency_address, boundary);
}

symbol_value address_resolver::resolve(dependency_solver&) const
{
    if (!dependency_address.bases().empty())
        return dependency_address;
    else
        return dependency_address.offset();
}

address address_resolver::extract_dep_address(const address& addr, size_t boundary)
{
    auto [spaces, _] = addr.normalized_spaces();
    auto enough = std::find_if(spaces.rbegin(), spaces.rend(), [boundary](const auto& e) {
        return e.first->kind == space_kind::ALIGNMENT && e.first->align.boundary >= boundary
            || e.first->kind == space_kind::LOCTR_SET || e.first->kind == space_kind::LOCTR_MAX;
    });
    if (enough != spaces.rend())
        spaces.erase(spaces.begin(), std::prev(enough.base()));
    return address({}, 0, address::space_list(std::make_shared<std::vector<address::space_entry>>(std::move(spaces))));
}

alignable_address_resolver::alignable_address_resolver(
    address dependency_address, address base_addrs, size_t boundary, int offset)
    : address_resolver(std::move(dependency_address), boundary)
    , base_addrs(std::move(base_addrs))
    , offset(offset)
{}

symbol_value alignable_address_resolver::resolve(dependency_solver&) const { return resolve(base_addrs); }

symbol_value alignable_address_resolver::resolve(const address& addr) const
{
    auto al = boundary ? (boundary - addr.offset() % boundary) % boundary : 0;
    return addr.offset() + (symbol_value::abs_value_t)al + offset;
}

alignable_address_abs_part_resolver::alignable_address_abs_part_resolver(const resolvable* dependency_source)
    : dependency_source_(dependency_source)
{}

dependency_collector alignable_address_abs_part_resolver::get_dependencies(dependency_solver& solver) const
{
    auto deps = dependency_source_->get_dependencies(solver);
    deps.unresolved_address = std::nullopt;
    return deps;
}

symbol_value alignable_address_abs_part_resolver::resolve(dependency_solver& solver) const
{
    auto result = dependency_source_->resolve(solver);

    if (result.value_kind() != symbol_value_kind::RELOC)
        result = symbol_value();

    return result;
}

aggregate_address_resolver::aggregate_address_resolver(std::vector<address> base_addrs, size_t boundary, int offset)
    : last_base_addrs(base_addrs.size() - 1)
    , base_addrs(std::move(base_addrs))
    , boundary(boundary)
    , offset(offset)
{}

symbol_value aggregate_address_resolver::resolve(dependency_solver&) const
{
    int max = -1;
    size_t idx = 0;
    for (size_t i = 0; i < base_addrs.size(); ++i)
    {
        if (base_addrs[i].offset() > max)
        {
            max = base_addrs[i].offset();
            idx = i;
        }
    }

    auto al = boundary ? (boundary - base_addrs[idx].offset() % boundary) % boundary : 0;
    return base_addrs[idx].offset() + (symbol_value::abs_value_t)al + offset;
}

dependency_collector aggregate_address_resolver::get_dependencies(dependency_solver&) const
{
    while (last_base_addrs != -1)
    {
        auto addr = address_resolver::extract_dep_address(base_addrs[last_base_addrs], boundary);
        if (addr.has_unresolved_space())
            return std::move(addr);

        --last_base_addrs;
    }
    return dependency_collector();
}
