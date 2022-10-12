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

#include "location_counter.h"

#include <algorithm>
#include <assert.h>
#include <stdexcept>

#include "section.h"

using namespace hlasm_plugin::parser_library::context;

bool location_counter::has_unresolved_spaces() const { return org_data_.back().has_space(); }

size_t location_counter::storage() const { return curr_data().storage; }

location_counter::location_counter(id_index name, const section& owner, loctr_kind kind)
    : switched_(nullptr)
    , layuot_created_(false)
    , name(name)
    , owner(owner)
    , kind(kind)
{
    org_data_.emplace_back(loctr_data_kind::POTENTIAL_MAX);

    if (kind == loctr_kind::NONSTARTING)
        (void)register_space(context::no_align, space_kind::LOCTR_BEGIN);
}

address location_counter::current_address() const
{
    return address({ &owner, nullptr }, curr_data().storage, curr_data().spaces());
}

address location_counter::current_address_for_alignment_evaluation(alignment align) const
{
    const auto& spaces = curr_data().unknown_parts;
    auto it = std::find_if(spaces.rbegin(), spaces.rend(), [align](const auto& up) {
        return up.unknown_space->align.boundary >= align.boundary;
    }).base();
    if (it == spaces.begin())
        return address({ &owner, nullptr }, curr_data().storage, curr_data().spaces());

    space_storage alignment_spaces;
    alignment_spaces.reserve(std::distance(it, spaces.end()));

    int offset = std::prev(it)->unknown_space->align.byte + std::prev(it)->storage_after;

    for (; it != spaces.end(); ++it)
    {
        offset += it->storage_after;
        alignment_spaces.push_back(it->unknown_space);
    }
    return address({ &owner, nullptr }, offset, alignment_spaces);
}

aligned_addr location_counter::reserve_storage_area(size_t length, alignment a)
{
    space_ptr sp = nullptr;
    if (!curr_data().has_alignment(a))
    {
        if (curr_data().need_space_alignment(a))
            sp = register_align_space(a);
        else
            curr_data().align(a);
    }

    curr_data().append_storage(length);

    check_available_value();

    return std::make_pair(address({ &owner, nullptr }, (int)curr_data().storage, curr_data().spaces()), sp);
}

aligned_addr location_counter::align(alignment align) { return reserve_storage_area(0, align); }

space_ptr location_counter::register_ordinary_space(alignment align)
{
    return register_space(align, space_kind::ORDINARY);
}

space_ptr location_counter::register_align_space(alignment align)
{
    return register_space(align, space_kind::ALIGNMENT);
}

bool location_counter::need_space_alignment(alignment align) const { return curr_data().need_space_alignment(align); }

space_ptr location_counter::set_value(const address& addr, size_t boundary, int offset, bool has_undefined_part)
{
    int al = boundary ? (int)((boundary - (addr.offset() % boundary)) % boundary) : 0;

    auto curr_addr = current_address();

    // checks whether addr is in this location counter and if it is well formed
    if (!addr.in_same_loctr(curr_addr) || (!addr.has_dependant_space() && addr.offset() + al + offset < 0))
        throw std::runtime_error("set incompatible loctr value");

    if (has_undefined_part)
    {
        // when address has undefined absolute part, register space
        org_data_.emplace_back();
        return register_space(context::no_align, boundary, offset);
    }

    if (curr_addr.spaces() != addr.spaces() || curr_data().storage - addr.offset() > curr_data().current_safe_area
        || (boundary && curr_data().storage - addr.offset() > curr_data().current_safe_area + offset))
    {
        // when addr is composed of different spaces or falls outside safe area, register space
        org_data_.emplace_back();
        return register_space(context::no_align, space_kind::LOCTR_SET);
    }
    else
    {
        int diff = addr.offset() - curr_data().storage;
        if (diff < 0 && curr_data().kind == loctr_data_kind::POTENTIAL_MAX)
        {
            // when value of addr is lower than loctr's and current loctr value is at its maximum, create new loctr data
            // so that we can track new loctr value for later maximum retrieval (in method set_available_value)
            org_data_.emplace_back(curr_data());
            curr_data().kind = loctr_data_kind::UNKNOWN_MAX;
        }
        curr_data().append_storage(diff);
        check_available_value();
        return nullptr;
    }
}

std::pair<space_ptr, std::vector<address>> location_counter::set_available_value()
{
    // here we find the loctr data with the highest value

    // erase last loctr data, if its value is lower than the previous one (and has the UNKNOWN_MAX enum)
    if (curr_data().kind == loctr_data_kind::UNKNOWN_MAX && (org_data_.end() - 2)->storage >= curr_data().storage)
        org_data_.erase(org_data_.end() - 1);

    // if we have only one loctr data with no spaces (or nonstarting loctr space),
    // we are already at the highest loctr value
    if (org_data_.size() == 1
        && (org_data_.front().unknown_parts.empty()
            || (org_data_.front().unknown_parts.size() == 1 && kind == loctr_kind::NONSTARTING)))
        return std::make_pair(nullptr, std::vector<address> {});

    // otherwise we collect representing addresses from all loctr values
    // and register space that will represent the highest loctr value

    std::vector<address> addr_arr;

    for (auto& entry : org_data_)
        addr_arr.emplace_back(address::base { &owner, nullptr }, entry.storage, entry.spaces());

    space_ptr loctr_start = nullptr;
    if (kind == loctr_kind::NONSTARTING)
    {
        loctr_start = addr_arr.front().spaces().front().first;
        assert(loctr_start->kind == space_kind::LOCTR_BEGIN);
        for (auto& addr : addr_arr)
        {
            // make addresses (pseudo-)relative to current location counter
            if (addr.spaces().front().first->kind == space_kind::LOCTR_BEGIN)
                addr.spaces().erase(addr.spaces().begin());
            else if (addr.spaces().front().first->kind == space_kind::LOCTR_SET)
                addr.spaces().emplace_back(loctr_start, -1);
            else if (addr.spaces().front().first->kind == space_kind::LOCTR_UNKNOWN)
                addr.spaces().emplace_back(loctr_start, -1);
        }
    }

    org_data_.emplace_back(loctr_data_kind::POTENTIAL_MAX);
    if (kind == loctr_kind::NONSTARTING)
        curr_data().append_space(std::move(loctr_start));
    (void)register_space(context::no_align, space_kind::LOCTR_MAX);

    return std::make_pair(curr_data().last_space(), std::move(addr_arr));
}

location_counter_data& location_counter::curr_data() { return org_data_.back(); }

const location_counter_data& location_counter::curr_data() const { return org_data_.back(); }

void location_counter::check_available_value()
{
    if (curr_data().kind == loctr_data_kind::POTENTIAL_MAX)
        return;

    assert(org_data_.size() > 1);
    if (check_if_higher_value(org_data_.size() - 1))
    {
        auto old_kind = (org_data_.end() - 2)->kind;
        org_data_.erase(org_data_.end() - 2);
        curr_data().kind = old_kind;
    }
}

bool location_counter::check_if_higher_value(size_t idx) const
{
    return org_data_[idx - 1].storage <= org_data_[idx].storage;
}

void location_counter::finish_layout(size_t offset)
{
    if (layuot_created_)
        throw std::runtime_error("layout already created");

    assert(!(kind == loctr_kind::STARTING) || offset == 0); // (STARTING => offset==0) <=> (!STARTING v offset==0)

    assert(!(kind == loctr_kind::NONSTARTING) || org_data_.front().fist_space()->kind == space_kind::LOCTR_BEGIN);

    if (kind == loctr_kind::NONSTARTING)
        space::resolve(org_data_.front().fist_space(), (int)offset);

    layuot_created_ = true;
}

void location_counter::resolve_space(space_ptr sp, int length)
{
    if (sp->kind == space_kind::LOCTR_MAX && !org_data_.empty())
    {
        auto it = std::find_if(std::next(org_data_.begin()), org_data_.end(), [s = sp.get()](const auto& d) {
            return d.matches_first_space(s);
        });
        if (it != org_data_.end())
            org_data_.erase(org_data_.begin(), std::prev(it));
    }
    for (auto& data : org_data_)
    {
        data.resolve_space(sp.get(), (size_t)length);
    }
}

void location_counter::switch_to_unresolved_value(space_ptr sp)
{
    assert(!switched_);

    auto it = std::find_if(
        org_data_.begin(), org_data_.end(), [s = sp.get()](const auto& d) { return d.matches_first_space(s); });

    assert(it != org_data_.end());

    switched_ = std::move(sp);

    switched_org_data_.insert(
        switched_org_data_.end(), std::make_move_iterator(it), std::make_move_iterator(org_data_.end()));

    org_data_.erase(it, org_data_.end());
}

std::variant<space_ptr, address> location_counter::restore_from_unresolved_value(space_ptr sp)
{
    assert(switched_ == sp);

    std::variant<space_ptr, address> result = curr_data().fist_space();
    size_t tmp_idx = org_data_.size() - 1;

    if (const auto& new_sp = std::get<space_ptr>(result); new_sp && new_sp->kind == space_kind::LOCTR_SET)
    {
        for (auto& switched_data : switched_org_data_)
        {
            if (switched_data.matches_first_space(sp.get()))
                org_data_.emplace_back(std::move(switched_data)).unknown_parts.front().unknown_space = new_sp;
            else
                org_data_.emplace_back(std::move(switched_data));
        }
    }
    else
    {
        result = current_address();

        for (auto& switched_data : switched_org_data_)
        {
            if (switched_data.matches_first_space(sp.get()))
            {
                auto& last = org_data_.emplace_back(org_data_[tmp_idx]);
                last.kind = switched_data.kind;
                last.append_data(std::move(switched_data));
                check_available_value();
            }
            else
                org_data_.emplace_back(std::move(switched_data));
        }
    }

    switched_ = nullptr;
    switched_org_data_.clear();

    if (check_if_higher_value(tmp_idx + 1))
        org_data_.erase(org_data_.begin() + tmp_idx);
    return result;
}

bool location_counter::check_underflow()
{
    int* checked_storage;
    bool ok = true;
    for (auto& data : org_data_)
    {
        if (data.fist_space() && data.fist_space()->kind == space_kind::LOCTR_BEGIN)
            checked_storage = &data.unknown_parts.front().storage_after;
        else if (!data.fist_space())
            checked_storage = &data.initial_storage;
        else
            continue;

        if (*checked_storage < 0)
        {
            *checked_storage = 0;
            ok = false;
        }
    }
    return ok;
}

space_ptr location_counter::register_space(alignment align, space_kind sp_kind)
{
    curr_data().append_space(std::make_shared<space>(*this, align, sp_kind));

    curr_data().kind = loctr_data_kind::POTENTIAL_MAX;

    return curr_data().last_space();
}

space_ptr location_counter::register_space(alignment align, size_t boundary, int offset)
{
    curr_data().append_space(std::make_shared<space>(*this, align, boundary, offset));

    curr_data().kind = loctr_data_kind::POTENTIAL_MAX;

    return curr_data().last_space();
}

bool location_counter::has_alignment(alignment align) const { return curr_data().has_alignment(align); }
