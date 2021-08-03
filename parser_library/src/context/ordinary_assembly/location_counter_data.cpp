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
#include <assert.h>
#include <stdexcept>

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
    unknown_parts.emplace_back(space_storage_t { sp, 0 });
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
    append_storage(data.unknown_parts.front().storage_after);

    unknown_parts.insert(unknown_parts.end(),
        std::make_move_iterator(data.unknown_parts.begin() + 1), // the first unknown part is substitiuted by this data
        std::make_move_iterator(data.unknown_parts.end()));

    current_safe_area = data.current_safe_area;
}

void location_counter_data::resolve_space(space_ptr sp, size_t length)
{
    size_t i = 0;
    for (auto it = unknown_parts.begin(); it != unknown_parts.end(); ++it, ++i)
    {
        if (it->unknown_space == sp)
        {
            storage += length;

            if (i == 0)
                initial_storage += it->storage_after + length;
            else
            {
                auto tmp = it;
                (--tmp)->storage_after += it->storage_after + length;
            }
            unknown_parts.erase(it);

            return;
        }
    }
}

void location_counter_data::resolve_space(space_ptr sp, space_ptr new_space)
{
    for (auto it = unknown_parts.begin(); it != unknown_parts.end(); ++it)
        if (it->unknown_space == sp)
        {
            it->unknown_space = std::move(new_space);
            return;
        }
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

space_ptr location_counter_data::fist_space() const
{
    if (unknown_parts.size())
        return unknown_parts.front().unknown_space;
    else
        return nullptr;
}

space_ptr location_counter_data::last_space() const
{
    if (unknown_parts.size())
        return unknown_parts.back().unknown_space;
    else
        return nullptr;
}

space_storage location_counter_data::spaces() const
{
    space_storage res;
    for (auto e : unknown_parts)
        res.push_back(e.unknown_space);
    return res;
}
