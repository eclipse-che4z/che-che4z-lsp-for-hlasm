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

    this_space->resolved_ptrs = std::move(value.spaces());

    this_space->resolved_length = value.offset();

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

int address::offset()
{
    int offs = offset_;
    for (const auto& s : spaces_)
        offs += get_space_offset(s.first);
    return offs;
}

int address::offset() const
{
    int offs = offset_;
    for (const auto& s : spaces_)
        offs += get_space_offset(s.first);
    return offs;
}

std::vector<address::space_entry>& address::spaces()
{
    // refresh();
    return spaces_;
}

std::pair<int, std::vector<address::space_entry>> get_unresolved_spaces(const address::space_entry& sp)
{
    int offset = 0;
    std::vector<address::space_entry> spaces;
    if (sp.first->resolved())
    {
        offset += sp.first->resolved_length;
        for (const auto& s : sp.first->resolved_ptrs)
        {
            auto [new_offs, new_spaces] = get_unresolved_spaces(s);
            offset += new_offs;
            spaces.insert(spaces.end(), new_spaces.begin(), new_spaces.end());
        }
    }
    else
        spaces.push_back(sp);

    return std::make_pair(offset, std::move(spaces));
}

const std::vector<address::space_entry>& address::spaces() const
{
    /*
    std::vector<space_entry> res_spaces;
    for (const auto& sp : spaces_)
    {
        auto [_, spcs] = get_unresolved_spaces(sp);
        res_spaces.insert(res_spaces.end(), spcs.begin(), spcs.end());
    }
    return res_spaces;*/
    return spaces_;
}

std::vector<address::space_entry> address::normalized_spaces() const
{
    std::vector<space_entry> res_spaces;
    for (const auto& sp : spaces_)
    {
        auto [_, spcs] = get_unresolved_spaces(sp);
        res_spaces.insert(res_spaces.end(), spcs.begin(), spcs.end());
    }
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
    refresh();
}

enum class op
{
    ADD,
    SUB
};

template<typename T>
bool compare(const T& lhs, const T& rhs)
{
    return lhs == rhs;
}
template<>
bool compare<address::base>(const address::base& lhs, const address::base& rhs)
{
    return lhs.owner == rhs.owner;
}

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
        auto it =
            std::find_if(prhs.begin(), prhs.end(), [&](auto e) { return e ? compare(entry.first, e->first) : false; });

        if (it != prhs.end())
        {
            int count;
            if (operation == op::ADD)
                count = (*it)->second + entry.second;
            else
                count = (*it)->second - entry.second;

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
        offset_ + addr.offset_,
        merge_entries(spaces_, addr.spaces_, op::ADD));
}

address address::operator+(int offs) const { return address(bases_, offset_ + offs, spaces_); }

address address::operator-(const address& addr) const
{
    return address(merge_entries(bases_, addr.bases_, op::SUB),
        offset_ - addr.offset_,
        merge_entries(spaces_, addr.spaces_, op::SUB));
}

address address::operator-(int offs) const { return address(bases_, offset_ - offs, spaces_); }

address address::operator-() const
{
    return address(merge_entries({}, bases_, op::SUB), -offset_, merge_entries({}, spaces_, op::SUB));
}

bool address::is_complex() const { return bases_.size() > 1; }

bool address::in_same_loctr(const address& addr) const
{
    if (!is_simple() || !addr.is_simple())
        return false;

    if (addr.bases_[0].first.owner != bases_[0].first.owner)
        return false;

    int counter = (addr.spaces_.size() && addr.spaces_[0].first->kind == space_kind::LOCTR_BEGIN)
        + (spaces_.size() && spaces_[0].first->kind == space_kind::LOCTR_BEGIN);

    if (counter == 2)
        return addr.spaces_[0].first == spaces_[0].first;
    else if (counter == 0)
        return true;
    else
        return false;
}

bool address::is_simple() const { return bases_.size() == 1 && bases_[0].second == 1; }

bool has_dependant_spaces(const space_ptr sp)
{
    if (!sp->resolved())
        return true;
    for (const auto& [s, _] : sp->resolved_ptrs)
        if (has_dependant_spaces(s))
            return true;
    return false;
}

bool address::has_dependant_space() const
{
    for (size_t i = 0; i < spaces_.size(); i++)
    {
        if (i == 0 && spaces_[i].first->kind != space_kind::LOCTR_BEGIN)
            continue;
        if (has_dependant_spaces(spaces_[i].first))
            return true;
    }
    return false;
}

address::address(std::vector<base_entry> bases_, int offset_, std::vector<space_entry> spaces_)
    : bases_(std::move(bases_))
    , offset_(offset_)
    , spaces_(std::move(spaces_))
{
    refresh();
}

void address::refresh()
{
    std::vector<space_entry> tmp_spaces;
    for (const auto& sp : spaces_)
    {
        auto [off, spcs] = get_unresolved_spaces(sp);
        offset_ += off;
        tmp_spaces.insert(tmp_spaces.end(), spcs.begin(), spcs.end());
    }
    spaces_ = std::move(tmp_spaces);
}

} // namespace hlasm_plugin::parser_library::context
