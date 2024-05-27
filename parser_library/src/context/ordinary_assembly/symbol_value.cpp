/*
 * Copyright (c) 2022 Broadcom.
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

#include <algorithm>
#include <cassert>
#include <numeric>

#include "symbol.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::context;

symbol_value::symbol_value(abs_value_t value)
    : value_(value)
{}

symbol_value::symbol_value(reloc_value_t value)
    : value_(std::move(value))
{}

symbol_value::symbol_value() = default;

symbol_value symbol_value::operator+(const symbol_value& value) const
{
    if (value_kind() == symbol_value_kind::UNDEF || value.value_kind() == symbol_value_kind::UNDEF)
        return symbol_value();

    if (value_kind() == symbol_value_kind::ABS)
    {
        if (value.value_kind() == symbol_value_kind::ABS)
            return get_abs() + value.get_abs();
        else
            return value.get_reloc() + get_abs();
    }
    else if (value_kind() == symbol_value_kind::RELOC)
    {
        if (value.value_kind() == symbol_value_kind::ABS)
            return get_reloc() + value.get_abs();
        else
        {
            auto tmp_val(get_reloc() + value.get_reloc());
            if (tmp_val.bases().empty() && !tmp_val.has_unresolved_space())
                return tmp_val.offset();
            else
                return tmp_val;
        }
    }
    else
    {
        assert(false);
        return symbol_value();
    }
}

symbol_value symbol_value::operator-(const symbol_value& value) const
{
    if (value_kind() == symbol_value_kind::UNDEF || value.value_kind() == symbol_value_kind::UNDEF)
        return symbol_value();

    if (value_kind() == symbol_value_kind::ABS)
    {
        if (value.value_kind() == symbol_value_kind::ABS)
            return get_abs() - value.get_abs();
        else
            return -value.get_reloc() + get_abs();
    }
    else if (value_kind() == symbol_value_kind::RELOC)
    {
        if (value.value_kind() == symbol_value_kind::ABS)
            return get_reloc() - value.get_abs();
        else
        {
            auto tmp_val(get_reloc() - value.get_reloc());
            if (tmp_val.bases().empty() && !tmp_val.has_unresolved_space())
                return tmp_val.offset();
            else
                return tmp_val;
        }
    }
    else
    {
        assert(false);
        return symbol_value();
    }
}

symbol_value symbol_value::operator*(const symbol_value& value) const
{
    if (value_kind() == symbol_value_kind::ABS && value.value_kind() == symbol_value_kind::ABS)
        return get_abs() * value.get_abs();

    return symbol_value();
}

symbol_value symbol_value::operator/(const symbol_value& value) const
{
    if (value_kind() == symbol_value_kind::ABS && value.value_kind() == symbol_value_kind::ABS)
        return value.get_abs() == 0 ? 0 : get_abs() / value.get_abs();

    return symbol_value();
}

symbol_value symbol_value::operator-() const
{
    switch (value_kind())
    {
        case symbol_value_kind::ABS:
            return -get_abs();
        case symbol_value_kind::RELOC:
            return -get_reloc();
        case symbol_value_kind::UNDEF:
            return symbol_value();
        default:
            assert(false);
            return symbol_value();
    }
}

const symbol_value::abs_value_t& symbol_value::get_abs() const { return std::get<abs_value_t>(value_); }

const symbol_value::reloc_value_t& symbol_value::get_reloc() const& { return std::get<reloc_value_t>(value_); }
symbol_value::reloc_value_t&& symbol_value::get_reloc() && { return std::get<reloc_value_t>(std::move(value_)); }

symbol_value_kind symbol_value::value_kind() const { return static_cast<symbol_value_kind>(value_.index()); }

template<typename It, typename Equal, typename Accumulate>
It aggregate(It begin, It end, Equal eq, Accumulate a)
{
    begin = std::adjacent_find(begin, end, eq); // identify first non-trivial group

    auto group_start = begin;
    while (group_start != end)
    {
        if (begin != group_start)
            *begin = std::move(*group_start);

        auto next_group = std::next(group_start);
        while (next_group != end)
        {
            if (!eq(*begin, *next_group))
                break;
            a(*begin, *next_group);
            ++next_group;
        }

        group_start = next_group;
        ++begin;
    }
    return begin;
}

symbol_value symbol_value::ignore_qualification() const
{
    if (value_kind() != symbol_value_kind::RELOC)
        return *this;

    auto result = get_reloc();
    if (std::ranges::all_of(result.bases(), [](const auto& be) { return be.first.qualifier.empty(); }))
    {
        if (result.bases().empty() && !result.has_unresolved_space())
            return result.offset();
        else
            return std::move(result);
    }

    auto bases = std::make_shared<std::vector<address::base_entry>>(result.bases().begin(), result.bases().end());

    std::ranges::for_each(*bases, [](auto& e) { e.first.qualifier = id_index(); });

    std::ranges::sort(*bases, {}, [](const auto& e) { return e.first.owner; });

    bases->erase(aggregate(
                     bases->begin(),
                     bases->end(),
                     [](const auto& l, const auto& r) { return l.first.owner == r.first.owner; },
                     [](auto& t, const auto& e) { t.second += e.second; }),
        bases->end());

    std::erase_if(*bases, [](const auto& e) { return e.second == 0; });

    if (bases->empty() && !result.has_unresolved_space())
        return result.offset();
    else
        return std::move(result).with_base_list(address::base_list(std::move(bases)));
}
