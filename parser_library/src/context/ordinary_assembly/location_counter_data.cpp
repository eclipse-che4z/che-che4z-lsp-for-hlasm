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

#include "location_counter_data.h"

#include <algorithm>
#include <bit>
#include <cassert>

using namespace hlasm_plugin::parser_library::context;

location_counter_data::location_counter_data()
    : location_counter_data(loctr_data_kind::UNKNOWN_MAX)
{}

location_counter_data::location_counter_data(loctr_data_kind kind)
    : storage(0)
    , initial_storage(0)
    , current_safe_area(0)
    , kind(kind)
{}

void location_counter_data::append_space(space_ptr sp)
{
    if (cached_spaces_for_address && cached_spaces_for_address->size() == cached_spaces_for_address->capacity())
        cached_spaces_for_address.reset();
    cached_pseudo_relative_spaces_for_address.reset();

    unknown_parts.emplace_back(space_storage_t { std::move(sp), 0 });
    if (cached_spaces_for_address)
        cached_spaces_for_address->push_back({ unknown_parts.back().unknown_space, 1 });
    current_safe_area = 0;
}

void location_counter_data::append_storage(int st)
{
    storage += st;
    if (unknown_parts.size())
        unknown_parts.back().storage_after += st;
    else
        initial_storage += st;
    current_safe_area += st;
}

void location_counter_data::append_data(location_counter_data data)
{
    cached_spaces_for_address.reset();
    cached_pseudo_relative_spaces_for_address.reset();

    append_storage(data.unknown_parts.front().storage_after);

    data.unknown_parts.pop_front(); // the first unknown part is substitiuted by this data

    unknown_parts.splice(unknown_parts.end(), data.unknown_parts);

    current_safe_area = data.current_safe_area;
}

void location_counter_data::resolve_space(const space* sp, size_t length)
{
    auto match = std::ranges::find(unknown_parts, sp, [](const auto& p) { return p.unknown_space.get(); });
    if (match == unknown_parts.end())
        return;

    cached_spaces_for_address.reset();
    cached_pseudo_relative_spaces_for_address.reset();

    storage += length;

    if (match == unknown_parts.begin())
        initial_storage += match->storage_after + length;
    else
        std::prev(match)->storage_after += match->storage_after + length;

    unknown_parts.erase(match);
}

void location_counter_data::resolve_space(const space* sp, space_ptr new_space)
{
    auto match = std::ranges::find(unknown_parts, sp, [](const auto& p) { return p.unknown_space.get(); });
    if (match == unknown_parts.end())
        return;

    cached_spaces_for_address.reset();
    cached_pseudo_relative_spaces_for_address.reset();
    match->unknown_space = std::move(new_space);
}

bool location_counter_data::has_alignment(alignment align) const
{
    size_t test;
    if (unknown_parts.empty())
        test = storage;
    else
    {
        if (align.boundary > unknown_parts.back().unknown_space->align.boundary)
            return false;
        test = unknown_parts.back().unknown_space->align.boundary + unknown_parts.back().unknown_space->align.byte
            + unknown_parts.back().storage_after;
    }

    return test % align.boundary == align.byte;
}

bool location_counter_data::align(alignment align)
{
    if (need_space_alignment(align))
        return false;

    auto tmp = (int)(((align.boundary - (last_storage() % align.boundary)) + align.byte) % align.boundary);

    append_storage(tmp);

    return true;
}

bool location_counter_data::need_space_alignment(alignment align) const
{
    return unknown_parts.size() && unknown_parts.back().unknown_space->align.boundary < align.boundary;
}

int location_counter_data::last_storage() const
{
    if (unknown_parts.empty())
        return initial_storage;
    else
        return unknown_parts.back().storage_after;
}

bool location_counter_data::matches_first_space(const space* s) const
{
    if (!unknown_parts.empty())
        return s == unknown_parts.front().unknown_space.get();
    else
        return false;
}

bool location_counter_data::has_space() const
{
    if (!unknown_parts.empty())
        return !!unknown_parts.front().unknown_space;
    else
        return false;
}

space_ptr location_counter_data::fist_space() const
{
    if (!unknown_parts.empty())
        return unknown_parts.front().unknown_space;
    else
        return nullptr;
}

space_ptr location_counter_data::last_space() const
{
    if (!unknown_parts.empty())
        return unknown_parts.back().unknown_space;
    else
        return nullptr;
}

address::space_list location_counter_data::spaces_for_address() const
{
    if (cached_spaces_for_address)
        return address::space_list(cached_spaces_for_address);

    auto result = std::make_shared<std::vector<address::space_entry>>();
    result->reserve(std::bit_ceil(unknown_parts.size() + unknown_parts.size() / 2));
    for (const auto& e : unknown_parts)
        result->push_back({ e.unknown_space, 1 });

    cached_spaces_for_address = result;

    return address::space_list(std::move(result));
}

address::space_list location_counter_data::pseudo_relative_spaces(space_ptr loctr_start) const
{
    if (cached_pseudo_relative_spaces_for_address)
        return *cached_pseudo_relative_spaces_for_address;

    auto result = std::make_shared<std::vector<address::space_entry>>();
    result->reserve(unknown_parts.size() + 1);
    for (const auto& e : unknown_parts)
        result->emplace_back(e.unknown_space, 1);

    const auto loctr_begin = loctr_start && result->front().first->kind == space_kind::LOCTR_BEGIN;
    const auto loctr_set = loctr_start && result->front().first->kind == space_kind::LOCTR_SET;
    const auto loctr_unknown = loctr_start && result->front().first->kind == space_kind::LOCTR_UNKNOWN;

    // make addresses (pseudo-)relative to current location counter
    if (loctr_set)
        result->push_back({ loctr_start, -1 });
    else if (loctr_unknown)
        result->push_back({ loctr_start, -1 });

    address::space_list result_list(std::move(result));

    // make addresses (pseudo-)relative to current location counter
    if (loctr_begin)
        result_list.spaces = result_list.spaces.subspan(1);

    cached_pseudo_relative_spaces_for_address = result_list;

    return result_list;
}
