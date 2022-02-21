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

#include "address.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

#include "location_counter.h"
#include "section.h"

namespace hlasm_plugin::parser_library::context {

space::space(location_counter& owner, alignment align, space_kind kind)
    : kind(kind)
    , align(std::move(align))
    , previous_loctr_value(address::base {}, 0, {})
    , owner(owner)
    , resolved_length(0)
    , resolved_(false)
{}

space::space(location_counter& owner, alignment align, address previous_loctr_value, size_t boundary, int offset)
    : kind(space_kind::LOCTR_UNKNOWN)
    , align(std::move(align))
    , previous_loctr_value(std::move(previous_loctr_value))
    , previous_boundary(boundary)
    , previous_offset(offset)
    , owner(owner)
    , resolved_length(0)
    , resolved_(false)
{}

void space::resolve(space_ptr this_space, int length)
{
    if (this_space->resolved_)
        return;

    if (this_space->kind == space_kind::ALIGNMENT)
    {
        alignment& align = this_space->align;
        if (length % align.boundary != align.byte)
            length = (int)((align.boundary - (length % align.boundary)) + align.byte) % align.boundary;
        else
            length = 0;
    }

    this_space->resolved_length = length;

    this_space->owner.resolve_space(this_space, length);

    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, space_ptr value)
{
    if (this_space->resolved_)
        return;

    assert(this_space->kind == space_kind::LOCTR_UNKNOWN);

    this_space->resolved_ptrs.push_back(address::space_entry(value, 1));

    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, address value)
{
    if (this_space->resolved_)
        return;

    assert(this_space->kind == space_kind::LOCTR_UNKNOWN);

    this_space->resolved_length = value.offset();

    this_space->resolved_ptrs = std::move(value.spaces());

    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, std::variant<space_ptr, address> value)
{
    if (std::holds_alternative<space_ptr>(value))
        resolve(std::move(this_space), std::move(std::get<space_ptr>(value)));
    else
        resolve(std::move(this_space), std::move(std::get<address>(value)));
}

bool space::resolved() const { return resolved_; }

std::string address::to_string() const
{
    std::stringstream ss;
    for (const auto& b : bases_)
    {
        if (*b.first.owner->name == "" || b.second == 0)
            continue;
        if (b.second > 1)
            ss << b.second << "*";
        if (b.first.qualifier)
            ss << *b.first.qualifier << ".";
        ss << *b.first.owner->name << " + ";
    }
    ss << std::to_string(offset());
    return ss.str();
}

const std::vector<address::base_entry>& address::bases() const { return bases_; }

std::vector<address::base_entry>& address::bases() { return bases_; }

int get_space_offset(space_ptr sp)
{
    int offset = 0;
    if (sp->resolved())
    {
        offset += sp->resolved_length;
        for (const auto& s : sp->resolved_ptrs)
            offset += get_space_offset(s.first);
    }
    return offset;
}

int address::offset() const
{
    int offs = offset_;
    for (const auto& s : spaces_)
        offs += get_space_offset(s.first);
    return offs;
}

std::vector<address::space_entry>& address::spaces() { return spaces_; }

void insert(const address::space_entry& sp,
    std::unordered_map<space_ptr, size_t>& normalized_map,
    std::vector<address::space_entry>& normalized_spaces)
{
    if (auto it = normalized_map.find(sp.first); it != normalized_map.end())
        normalized_spaces[it->second].second += sp.second;
    else
    {
        normalized_spaces.push_back(sp);
        normalized_map.emplace(sp.first, normalized_spaces.size() - 1);
    }
}

int get_unresolved_spaces(const std::vector<address::space_entry>& spaces,
    std::unordered_map<space_ptr, size_t>& normalized_map,
    std::vector<address::space_entry>& normalized_spaces)
{
    int offset = 0;
    for (const auto& sp : spaces)
    {
        if (sp.first->resolved())
        {
            offset += sp.first->resolved_length;
            offset += get_unresolved_spaces(sp.first->resolved_ptrs, normalized_map, normalized_spaces);
        }
        else
            insert(sp, normalized_map, normalized_spaces);
    }

    return offset;
}

const std::vector<address::space_entry>& address::spaces() const { return spaces_; }

std::vector<address::space_entry> address::normalized_spaces() const
{
    std::vector<space_entry> res_spaces;
    std::unordered_map<space_ptr, size_t> tmp_map;

    get_unresolved_spaces(spaces_, tmp_map, res_spaces);

    res_spaces.erase(
        std::remove_if(res_spaces.begin(), res_spaces.end(), [](const space_entry& e) { return e.second == 0; }),
        res_spaces.end());

    return res_spaces;
}

address::address(base address_base, int offset, const space_storage& spaces)
    : offset_(offset)
{
    bases_.emplace_back(address_base, 1);

    for (auto& space : spaces)
    {
        spaces_.emplace_back(space, 1);
    }
    normalize();
}

enum class op
{
    ADD,
    SUB
};

template<typename T>
std::vector<T> merge_entries(const std::vector<T>& lhs, const std::vector<T>& rhs, const op operation)
{
    std::vector<T> res;
    std::vector<const T*> prhs;

    prhs.reserve(rhs.size());
    for (const auto& e : rhs)
        prhs.push_back(&e);

    for (auto& entry : lhs)
    {
        auto it = std::find_if(prhs.begin(), prhs.end(), [&](auto e) { return e ? entry.first == e->first : false; });

        if (it != prhs.end())
        {
            int count;
            if (operation == op::ADD)
                count = entry.second + (*it)->second; // L + R
            else
                count = entry.second - (*it)->second; // L - R

            if (count != 0)
                res.emplace_back(entry.first, count);

            *it = nullptr;
        }
        else
        {
            res.push_back(entry);
        }
    }

    for (auto&& rest : prhs)
    {
        if (!rest)
            continue;

        res.push_back(*rest);
        if (operation == op::SUB)
            res.back().second *= -1;
    }

    return res;
}

address address::operator+(const address& addr) const
{
    return address(merge_entries(bases_, addr.bases_, op::ADD),
        offset() + addr.offset(),
        merge_entries(normalized_spaces(), addr.normalized_spaces(), op::ADD));
}

address address::operator+(int offs) const { return address(bases_, offset_ + offs, spaces_); }

address address::operator-(const address& addr) const
{
    return address(merge_entries(bases_, addr.bases_, op::SUB),
        offset() - addr.offset(),
        merge_entries(normalized_spaces(), addr.normalized_spaces(), op::SUB));
}

address address::operator-(int offs) const { return address(bases_, offset() - offs, normalized_spaces()); }

address address::operator-() const
{
    return address(merge_entries({}, bases_, op::SUB), -offset(), merge_entries({}, normalized_spaces(), op::SUB));
}

bool address::is_complex() const { return bases_.size() > 1; }

bool address::in_same_loctr(const address& addr) const
{
    if (!is_simple() || !addr.is_simple())
        return false;

    if (addr.bases_[0].first != bases_[0].first)
        return false;

    bool this_has_loctr_begin = spaces_.size() && spaces_[0].first->kind == space_kind::LOCTR_BEGIN;
    bool addr_has_loctr_begin = addr.spaces_.size() && addr.spaces_[0].first->kind == space_kind::LOCTR_BEGIN;

    if (this_has_loctr_begin && addr_has_loctr_begin)
        return addr.spaces_[0].first == spaces_[0].first;
    else if (!this_has_loctr_begin && !addr_has_loctr_begin)
        return true;
    else
    {
        if (addr.spaces_.size() && spaces_.size())
            return addr.spaces_.front().first->owner.name == spaces_.front().first->owner.name;
        return false;
    }
}

bool address::is_simple() const { return bases_.size() == 1 && bases_[0].second == 1; }

bool has_unresolved_spaces(const space_ptr sp)
{
    if (!sp->resolved())
        return true;
    for (const auto& [s, _] : sp->resolved_ptrs)
        if (has_unresolved_spaces(s))
            return true;
    return false;
}

bool address::has_dependant_space() const
{
    for (size_t i = 0; i < spaces_.size(); i++)
    {
        if (i == 0 && spaces_[i].first->kind == space_kind::LOCTR_BEGIN)
            continue;
        if (has_unresolved_spaces(spaces_[i].first))
            return true;
    }
    return false;
}

bool address::has_unresolved_space() const
{
    for (const auto& [sp, _] : spaces_)
        if (has_unresolved_spaces(sp))
            return true;
    return false;
}

address::address(std::vector<base_entry> bases_, int offset_, std::vector<space_entry> spaces_)
    : bases_(std::move(bases_))
    , offset_(offset_)
    , spaces_(std::move(spaces_))
{}

void address::normalize()
{
    std::vector<space_entry> res_spaces;
    std::unordered_map<space_ptr, size_t> tmp_map;

    offset_ += get_unresolved_spaces(spaces_, tmp_map, res_spaces);

    res_spaces.erase(
        std::remove_if(res_spaces.begin(), res_spaces.end(), [](const space_entry& e) { return e.second == 0; }),
        res_spaces.end());

    spaces_ = std::move(res_spaces);
}

} // namespace hlasm_plugin::parser_library::context
