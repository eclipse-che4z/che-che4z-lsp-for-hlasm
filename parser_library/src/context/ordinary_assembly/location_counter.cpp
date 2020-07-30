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

bool location_counter::has_unresolved_spaces() const { return !!org_data_.back().fist_space(); }

size_t location_counter::storage() const { return curr_data().storage; }

location_counter::location_counter(id_index name, const section& owner, const loctr_kind kind, id_storage& ids)
    : switched_(nullptr)
    , last_space_(0)
    , ids_(ids)
    , layuot_created_(false)
    , name(name)
    , owner(owner)
    , kind(kind)
{
    org_data_.emplace_back(loctr_data_kind::POTENTIAL_MAX);

    if (kind == loctr_kind::NONSTARTING)
        (void)register_space(context::no_align, space_kind::LOCTR_BEGIN);
}

address location_counter::current_address()
{
    return address({ &owner }, (int)curr_data().storage, curr_data().spaces());
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

    return std::make_pair(address({ &owner }, (int)curr_data().storage, curr_data().spaces()), sp);
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

    if (!addr.in_same_loctr(curr_addr) || (!addr.has_dependant_space() && addr.offset() + al + offset < 0))
        throw std::runtime_error("set incompatible loctr value");

    if (has_undefined_part)
    {
        org_data_.emplace_back();
        return register_space(context::no_align, std::move(curr_addr), boundary, offset);
    }

    if (curr_addr.spaces() != addr.spaces() || (int)curr_data().storage - addr.offset() > curr_data().current_safe_area
        || (boundary && (int)curr_data().storage - addr.offset() > curr_data().current_safe_area + offset))
    {
        org_data_.emplace_back();
        return register_space(context::no_align, space_kind::LOCTR_SET);
    }
    else
    {
        int diff = addr.offset() - curr_data().storage;
        if (diff < 0 && curr_data().kind == loctr_data_kind::POTENTIAL_MAX)
        {
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
    if (curr_data().kind == loctr_data_kind::UNKNOWN_MAX && (org_data_.end() - 2)->storage >= curr_data().storage)
        org_data_.erase(org_data_.end() - 1);

    if (org_data_.size() == 1
        && (org_data_.begin()->unknown_parts.empty()
            || (org_data_.begin()->unknown_parts.size() == 1 && kind == loctr_kind::NONSTARTING)))
        return std::make_pair(nullptr, std::vector<address> {});

    std::vector<address> addr_arr;

    for (auto& entry : org_data_)
        addr_arr.emplace_back(address::base { &owner }, entry.storage, entry.spaces());

    space_ptr loctr_start = nullptr;
    if (kind == loctr_kind::NONSTARTING)
    {
        loctr_start = addr_arr.front().spaces().front().first;
        assert(loctr_start->kind == space_kind::LOCTR_BEGIN);
        for (auto& addr : addr_arr)
        {
            if (addr.spaces().front().first->kind == space_kind::LOCTR_BEGIN)
                addr.spaces().erase(addr.spaces().begin());
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
    if (sp->kind == space_kind::LOCTR_MAX)
    {
        for (size_t i = 0; i < org_data_.size(); ++i)
        {
            if (org_data_[i].fist_space() == sp)
            {
                if (i > 0)
                    org_data_.erase(org_data_.begin(), org_data_.begin() + i - 1);
                break;
            }
        }
    }
    for (auto& data : org_data_)
    {
        data.resolve_space(sp, (size_t)length);
    }
}

void location_counter::switch_to_unresolved_value(space_ptr sp)
{
    if (switched_)
        throw std::runtime_error("value already switched");

    size_t new_size = 0;
    for (size_t i = 0; i < org_data_.size(); ++i)
    {
        if (org_data_[i].fist_space() && org_data_[i].fist_space() == sp && !switched_)
        {
            switched_ = sp;
            new_size = i;
        }
        if (switched_)
            switched_org_data_.emplace_back(std::move(org_data_[i]));
    }

    if (!switched_)
        throw std::runtime_error("value not found");

    org_data_.resize(new_size);
}

std::variant<space_ptr, address> location_counter::restore_from_unresolved_value(space_ptr sp)
{
    if (switched_ != sp)
        throw std::runtime_error("restoring bad value");

    size_t tmp_idx = org_data_.size() - 1;
    space_ptr new_sp;

    if (curr_data().fist_space() && curr_data().fist_space()->kind == space_kind::LOCTR_SET)
        new_sp = curr_data().fist_space();

    address new_addr = current_address();

    for (size_t i = 0; i < switched_org_data_.size(); ++i)
    {
        if (switched_org_data_[i].fist_space() && switched_org_data_[i].fist_space() == sp)
        {
            if (new_sp)
            {
                org_data_.emplace_back(std::move(switched_org_data_[i]));
                org_data_.back().unknown_parts.front().unknown_space = new_sp;
            }
            else
            {
                org_data_.emplace_back(org_data_[tmp_idx]);
                org_data_.back().kind = switched_org_data_[i].kind;

                org_data_.back().append_data(std::move(switched_org_data_[i]));
                check_available_value();
            }
        }
        else
            org_data_.emplace_back(std::move(switched_org_data_[i]));
    }
    switched_ = nullptr;
    switched_org_data_.clear();

    if (check_if_higher_value(tmp_idx + 1))
        org_data_.erase(org_data_.begin() + tmp_idx);

    if (new_sp)
        return new_sp;
    return new_addr;
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

id_index location_counter::create_space_name(char type)
{
    std::string tmp("_ " + *owner.name + " " + *name + " " + std::to_string(last_space_));

    ++last_space_;
    tmp[0] = type;

    return ids_.add(std::move(tmp));
}

space_ptr location_counter::register_space(alignment align, space_kind sp_kind)
{
    curr_data().append_space(std::make_shared<space>(*this, align, sp_kind));

    curr_data().kind = loctr_data_kind::POTENTIAL_MAX;

    return curr_data().last_space();
}

space_ptr location_counter::register_space(alignment align, address addr, size_t boundary, int offset)
{
    curr_data().append_space(std::make_shared<space>(*this, align, std::move(addr), boundary, offset));

    curr_data().kind = loctr_data_kind::POTENTIAL_MAX;

    return curr_data().last_space();
}

bool location_counter::has_alignment(alignment align) const { return curr_data().has_alignment(align); }
