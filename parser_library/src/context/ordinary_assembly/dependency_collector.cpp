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
#include <iterator>

#include "utils/merge_sorted.h"

using namespace hlasm_plugin::parser_library::context;

dependency_collector::dependency_collector() = default;

dependency_collector::dependency_collector(error)
    : has_error(true)
{}

dependency_collector::dependency_collector(id_index undefined_symbol)
{
    undefined_symbolics.emplace_back(undefined_symbol);
}

dependency_collector::dependency_collector(address u_a)
    : unresolved_address(std::move(u_a))
{}

dependency_collector::dependency_collector(attr_ref attribute_reference)
{
    undefined_symbolics.emplace_back(attribute_reference.symbol_id, attribute_reference.attribute);
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

namespace {
struct merge_spaces_comparator
{
    auto operator()(const space_ptr& l, const address::space_entry& r) const noexcept { return l <=> r.first; }
    auto operator()(const space_ptr& l, const space_ptr& r) const noexcept { return l <=> r; }
};
struct merge_spaces
{
    void operator()(space_ptr&, const address::space_entry&) const noexcept {}
    auto operator()(const address::space_entry& e) const noexcept { return e.first; }
};
struct name_comparator
{
    auto operator()(const id_index& l, const id_index& r) const noexcept { return l <=> r; }
    auto operator()(const id_index& l, const symbolic_reference& r) const noexcept { return l <=> r.name; }
    auto operator()(const symbolic_reference& l, const symbolic_reference& r) const noexcept
    {
        return l.name <=> r.name;
    }
};
struct name_merger
{
    void operator()(id_index&, const symbolic_reference&) const noexcept {}
    auto operator()(const symbolic_reference& e) const noexcept { return e.name; }
};
struct flags_merger
{
    void operator()(symbolic_reference& l, const symbolic_reference& r) const noexcept { l.flags |= r.flags; }
    auto operator()(const symbolic_reference& e) const noexcept { return e; }
};
} // namespace

dependency_collector& hlasm_plugin::parser_library::context::dependency_collector::merge(const dependency_collector& dc)
{
    merge_undef(dc);

    if (unresolved_address)
    {
        auto [spaces, _] = unresolved_address->normalized_spaces();
        utils::merge_unsorted(unresolved_spaces,
            std::make_move_iterator(spaces.begin()),
            std::make_move_iterator(spaces.end()),
            merge_spaces_comparator(),
            merge_spaces());
        unresolved_address.reset();
    }

    if (dc.unresolved_address)
    {
        auto [spaces, _] = dc.unresolved_address->normalized_spaces();
        utils::merge_unsorted(unresolved_spaces,
            std::make_move_iterator(spaces.begin()),
            std::make_move_iterator(spaces.end()),
            merge_spaces_comparator(),
            merge_spaces());
    }

    return *this;
}

bool dependency_collector::is_address() const
{
    return std::ranges::all_of(undefined_symbolics, [](const auto& e) { return !e.get(); }) && unresolved_address
        && !unresolved_address.value().bases().empty();
}

bool dependency_collector::contains_dependencies() const
{
    return !undefined_symbolics.empty() || !unresolved_spaces.empty()
        || (unresolved_address && unresolved_address->has_unresolved_space());
}

void dependency_collector::collect_unique_symbolic_dependencies(std::vector<context::id_index>& missing_symbols) const
{
    std::ranges::sort(missing_symbols);

    utils::merge_sorted(missing_symbols, undefined_symbolics, name_comparator(), name_merger());
}

bool dependency_collector::merge_undef(const dependency_collector& holder)
{
    has_error |= holder.has_error;

    utils::merge_sorted(undefined_symbolics, holder.undefined_symbolics, name_comparator(), flags_merger());

    utils::merge_sorted(unresolved_spaces, holder.unresolved_spaces, std::compare_three_way());

    return has_error || std::ranges::any_of(undefined_symbolics, [](const auto& e) { return e.get(); });
}

void dependency_collector::add_sub(const dependency_collector& holder, bool add)
{
    if (unresolved_address && holder.unresolved_address)
    {
        if (add)
            unresolved_address = *unresolved_address + (*holder.unresolved_address);
        else
            unresolved_address = *unresolved_address - (*holder.unresolved_address);
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
        {
            auto [spaces, _] = unresolved_address->normalized_spaces();
            utils::merge_unsorted(unresolved_spaces,
                std::make_move_iterator(spaces.begin()),
                std::make_move_iterator(spaces.end()),
                merge_spaces_comparator(),
                merge_spaces());
        }
        if (holder.unresolved_address)
        {
            auto [spaces, _] = holder.unresolved_address->normalized_spaces();
            utils::merge_unsorted(unresolved_spaces,
                std::make_move_iterator(spaces.begin()),
                std::make_move_iterator(spaces.end()),
                merge_spaces_comparator(),
                merge_spaces());
        }
    }
}
