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

using namespace hlasm_plugin::parser_library::context;

space::space(location_counter& owner, alignment align, space_kind kind)
    : kind(kind)
    , align(std::move(align))
    , previous_loctr_value(address::base {}, 0, {})
    , owner(owner)
    , resolved_(false)
{}

space::space(location_counter& owner, alignment align, address previous_loctr_value, size_t boundary, int offset)
    : kind(space_kind::LOCTR_UNKNOWN)
    , align(std::move(align))
    , previous_loctr_value(std::move(previous_loctr_value))
    , previous_boundary(boundary)
    , previous_offset(offset)
    , owner(owner)
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

    this_space->owner.resolve_space(this_space, length);

    for (auto& listener : this_space->listeners_)
    {
        auto l_tmp = std::find_if(
            listener->spaces.begin(), listener->spaces.end(), [&](auto s) { return s.first == this_space; });

        assert(l_tmp != listener->spaces.end());

        listener->offset += length;

        listener->spaces.erase(l_tmp);
    }

    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, space_ptr value)
{
    if (this_space->resolved_)
        return;

    assert(this_space->kind == space_kind::LOCTR_UNKNOWN);

    for (int i = this_space->listeners_.size() - 1; i >= 0; --i)
    {
        auto listener = this_space->listeners_[i];
        assert(listener->spaces.front().first == this_space);

        listener->spaces.front().first->remove_listener(listener);
        listener->spaces.front().first = value;
        value->add_listener(listener);
    }

    this_space->listeners_.clear();
    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, address value)
{
    if (this_space->resolved_)
        return;

    assert(this_space->kind == space_kind::LOCTR_UNKNOWN);

    for (int i = this_space->listeners_.size() - 1; i >= 0; --i)
    {
        auto listener = this_space->listeners_[i];
        assert(listener->spaces.front().first == this_space);

        assert(listener->bases.size() == value.bases.size());
        listener->offset += value.offset;
        listener->spaces.erase(listener->spaces.begin());
        auto spaces = value.spaces;
        for (auto&& space : listener->spaces)
            spaces.push_back(std::move(space));
        listener->spaces = spaces;
        for (auto&& sp : value.spaces)
            sp.first->add_listener(listener);
    }

    for (auto& sp : value.spaces)
        sp.first->remove_listener(&value);
    value.spaces.clear();

    this_space->listeners_.clear();
    this_space->resolved_ = true;
}

void space::resolve(space_ptr this_space, std::variant<space_ptr, address> value)
{
    if (std::holds_alternative<space_ptr>(value))
        resolve(std::move(this_space), std::move(std::get<space_ptr>(value)));
    else
        resolve(std::move(this_space), std::move(std::get<address>(value)));
}

void space::add_listener(address* addr)
{
    assert(!resolved_);
    assert(std::find(listeners_.begin(), listeners_.end(), addr) == listeners_.end());
    assert(std::find_if(addr->spaces.begin(), addr->spaces.end(), [=](auto& sp) { return &*sp.first == this; })
        != addr->spaces.end());
    listeners_.push_back(addr);
}

void space::remove_listener(address* addr)
{
    auto it = std::find(listeners_.begin(), listeners_.end(), addr);
    assert(it != listeners_.end());
    listeners_.erase(it);
}

std::string address::to_string() const
{
    std::stringstream ss;
    for (const auto& b : bases)
    {
        if (*b.first.owner->name == "" || b.second == 0)
            continue;
        if (b.second > 1)
            ss << b.second << "*";
        ss << *b.first.owner->name << " + ";
    }
    ss << std::to_string(offset);
    return ss.str();
}

address::address(base address_base, int offset, const space_storage& spaces)
    : offset(offset)
{
    bases.emplace_back(address_base, 1);

    for (auto& space : spaces)
    {
        this->spaces.emplace_back(space, 1);
        space->add_listener(this);
    }
}

address::address(const address& addr)
    : address(addr.bases, addr.offset, addr.spaces)
{}

address& address::operator=(const address& addr)
{
    for (auto& [sp, count] : spaces)
        sp->remove_listener(this);

    bases = addr.bases;
    offset = addr.offset;
    spaces = addr.spaces;

    for (auto& [sp, count] : spaces)
        sp->add_listener(this);

    return *this;
}

address::address(address&& addr)
    : bases(std::move(addr.bases))
    , offset(addr.offset)
    , spaces(std::move(addr.spaces))
{
    for (auto& [sp, count] : spaces)
    {
        sp->add_listener(this);
        sp->remove_listener(&addr);
    }

    addr.spaces = {};
}

address& address::operator=(address&& addr)
{
    for (auto& [sp, count] : spaces)
        sp->remove_listener(this);

    bases = std::move(addr.bases);
    offset = std::move(addr.offset);
    spaces = std::move(addr.spaces);

    for (auto& [sp, count] : spaces)
    {
        sp->add_listener(this);
        sp->remove_listener(&addr);
    }

    addr.spaces = {};

    return *this;
}

enum class op
{
    ADD,
    SUB
};

template<typename T> bool compare(const T& lhs, const T& rhs) { return lhs == rhs; }
template<> bool compare<address::base>(const address::base& lhs, const address::base& rhs)
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
    return address(
        merge_entries(bases, addr.bases, op::ADD), offset + addr.offset, merge_entries(spaces, addr.spaces, op::ADD));
}

address address::operator+(int offs) const { return address(bases, offset + offs, spaces); }

address address::operator-(const address& addr) const
{
    return address(
        merge_entries(bases, addr.bases, op::SUB), offset - addr.offset, merge_entries(spaces, addr.spaces, op::SUB));
}

address address::operator-(int offs) const { return address(bases, offset - offs, spaces); }

address address::operator-() const
{
    return address(merge_entries({}, bases, op::SUB), -offset, merge_entries({}, spaces, op::SUB));
}

bool address::is_complex() const { return bases.size() > 1; }

bool address::in_same_loctr(const address& addr) const
{
    if (!is_simple() || !addr.is_simple())
        return false;

    if (addr.bases[0].first.owner != bases[0].first.owner)
        return false;

    int counter = (addr.spaces.size() && addr.spaces[0].first->kind == space_kind::LOCTR_BEGIN)
        + (spaces.size() && spaces[0].first->kind == space_kind::LOCTR_BEGIN);

    if (counter == 2)
        return addr.spaces[0].first == spaces[0].first;
    else if (counter == 0)
        return true;
    else
        return false;
}

bool address::is_simple() const { return bases.size() == 1 && bases[0].second == 1; }

bool address::has_dependant_space() const
{
    switch (spaces.size())
    {
        case 0:
            return false;
        case 1:
            return spaces.front().first->kind != space_kind::LOCTR_BEGIN;
        default:
            return true;
    }
}

address::~address()
{
    for (auto& [sp, count] : spaces)
        sp->remove_listener(this);
}

address::address(std::vector<base_entry> bases_, int offset_, std::vector<space_entry> spaces_)
    : bases(std::move(bases_))
    , offset(offset_)
    , spaces(std::move(spaces_))
{
    for (auto& [sp, count] : spaces)
    {
        assert(count != 0);
        sp->add_listener(this);
    }
}
