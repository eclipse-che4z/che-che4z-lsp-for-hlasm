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
#include <concepts>
#if __cpp_lib_polymorphic_allocator >= 201902L && __cpp_lib_memory_resource >= 201603L
#    include <memory_resource>
#endif
#include <numeric>
#include <span>
#include <type_traits>
#include <unordered_map>

#include "location_counter.h"
#include "section.h"
#include "utils/merge_sorted.h"

namespace hlasm_plugin::parser_library::context {

space::space(location_counter& owner, alignment align, space_kind kind)
    : kind(kind)
    , align(std::move(align))
    , owner(owner)
    , resolved_length(0)
    , resolved_(false)
{}

space::space(location_counter& owner, alignment align, size_t boundary, int offset)
    : kind(space_kind::LOCTR_UNKNOWN)
    , align(std::move(align))
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

void space::resolve(space_ptr this_space, int length, std::vector<address::space_entry> unresolved)
{
    if (this_space->resolved_)
        return;

    assert(this_space->kind == space_kind::LOCTR_UNKNOWN);

    this_space->resolved_length = length;

    this_space->resolved_ptrs = std::move(unresolved);

    this_space->resolved_ = true;
}

const std::vector<address::base_entry>& address::bases() const { return bases_; }

std::vector<address::base_entry>& address::bases() { return bases_; }

int get_space_offset(std::span<const address::space_entry> sp_vec)
{
    return std::transform_reduce(sp_vec.begin(), sp_vec.end(), 0, std::plus<>(), [](const auto& v) {
        const auto& [sp, cnt] = v;
        return sp->resolved() ? cnt * (sp->resolved_length + get_space_offset(sp->resolved_ptrs)) : 0;
    });
}

int address::offset() const { return offset_ + get_space_offset(spaces_.spaces); }
int address::unresolved_offset() const { return offset_; }

enum class merge_op : bool
{
    add,
    sub
};

struct normalization_helper
{
    normalization_helper(const normalization_helper&) = delete;
    normalization_helper(normalization_helper&&) = delete;
    normalization_helper& operator=(const normalization_helper&) = delete;
    normalization_helper& operator=(normalization_helper&&) = delete;
#if __cpp_lib_polymorphic_allocator >= 201902L && __cpp_lib_memory_resource >= 201603L
    normalization_helper()
        : buffer_resource(buffer.data(), buffer.size())
        , map(&buffer_resource)
    {}
    alignas(std::max_align_t) std::array<unsigned char, 8 * 1024> buffer;
    std::pmr::monotonic_buffer_resource buffer_resource;
    std::pmr::unordered_map<space*, size_t> map;
#else
    normalization_helper() = default;
    std::unordered_map<space*, size_t> map;
#endif
};

int get_unresolved_spaces(std::span<const address::space_entry> spaces,
    normalization_helper& helper,
    std::vector<address::space_entry>& normalized_spaces,
    int multiplier)
{
    int offset = 0;
    for (auto&& sp : spaces)
    {
        if (sp.first->resolved())
        {
            offset += multiplier * sp.second * sp.first->resolved_length
                + get_unresolved_spaces(sp.first->resolved_ptrs, helper, normalized_spaces, multiplier * sp.second);
            // TODO: overflow check
        }
        else if (auto [it, inserted] = helper.map.try_emplace(sp.first.get(), normalized_spaces.size()); inserted)
            normalized_spaces.emplace_back(sp).second *= multiplier;
        else
            normalized_spaces[it->second].second += multiplier * sp.second;
    }

    return offset;
}

void cleanup_spaces(std::vector<address::space_entry>& spaces)
{
    std::erase_if(spaces, [](const address::space_entry& e) { return e.second == 0; });
}

std::pair<std::vector<address::space_entry>, int> address::normalized_spaces(std::span<const space_entry> spaces)
{
    if (spaces.empty())
        return {};

    std::vector<space_entry> res_spaces;
    normalization_helper helper;

    int offset = get_unresolved_spaces(spaces, helper, res_spaces, 1);

    cleanup_spaces(res_spaces);

    return { std::move(res_spaces), offset };
}

std::pair<std::vector<address::space_entry>, int> address::normalized_spaces() const
{
    return normalized_spaces(spaces_.spaces);
}

address::address(base address_base, int offset, const space_storage& spaces)
    : offset_(offset)
{
    bases_.emplace_back(address_base, 1);

    if (spaces.empty())
        return;

    auto new_spaces = std::make_shared<std::vector<space_entry>>();
    new_spaces->reserve(spaces.size());
    for (auto& space : spaces)
        new_spaces->emplace_back(space, 1);

    spaces_.spaces = *new_spaces;
    spaces_.owner = std::move(new_spaces);
}

address::address(base address_base, int offset, space_storage&& spaces)
    : offset_(offset)
{
    bases_.emplace_back(address_base, 1);

    if (spaces.empty())
        return;

    auto new_spaces = std::make_shared<std::vector<space_entry>>();
    new_spaces->reserve(spaces.size());
    for (auto& space : spaces)
        new_spaces->emplace_back(std::move(space), 1);

    spaces_.spaces = *new_spaces;
    spaces_.owner = std::move(new_spaces);
}

template<merge_op operation>
std::vector<address::base_entry> merge_bases(
    const std::vector<address::base_entry>& l, const std::vector<address::base_entry>& r)
{
    if (r.empty())
        return l;

    if constexpr (operation == merge_op::add)
    {
        if (l.empty())
            return r;
    }
    else
    {
        if (l.empty())
        {
            auto result = r;
            for (auto& [_, cnt] : result)
                cnt *= -1;
            return result;
        }
        if (l == r)
            return {};
    }

    std::vector<address::base_entry> result;

    result.reserve(l.size() + r.size());

    result = r;
    if constexpr (operation == merge_op::sub)
    {
        for (auto& [_, cnt] : result)
            cnt *= -1;
    }

    std::sort(result.begin(), result.end(), [](const auto& l, const auto& r) { return l.first < r.first; });
    utils::merge_unsorted(
        result,
        l,
        [](const auto& l, const auto& r) { return l.first <=> r.first; },
        [](auto& r, const auto& e) { r.second += e.second; });

    std::erase_if(result, [](const auto& e) { return e.second == 0; });

    return result;
}

address address::operator+(const address& addr) const
{
    if (!has_spaces() && !addr.has_spaces())
        return address(merge_bases<merge_op::add>(bases_, addr.bases_), offset_ + addr.offset_, space_list());

    auto res_spaces = std::make_shared<std::vector<space_entry>>();
    normalization_helper helper;

    int offset = 0;
    offset += get_unresolved_spaces(spaces_.spaces, helper, *res_spaces, 1);
    offset += get_unresolved_spaces(addr.spaces_.spaces, helper, *res_spaces, 1);

    cleanup_spaces(*res_spaces);

    return address(merge_bases<merge_op::add>(bases_, addr.bases_),
        offset_ + addr.offset_ + offset,
        space_list(std::move(res_spaces)));
}

address address::operator+(int offs) const { return address(bases_, offset_ + offs, spaces_); }

std::pair<std::span<const address::space_entry>, std::span<const address::space_entry>> trim_common(
    std::span<const address::space_entry> l, std::span<const address::space_entry> r)
{
    if (l.data() == r.data())
    {
        auto common = std::min(l.size(), r.size());
        return { l.subspan(common), r.subspan(common) };
    }
    auto [le, re] = std::mismatch(l.begin(), l.end(), r.begin(), r.end());
    return { { le, l.end() }, { re, r.end() } };
}

address address::operator-(const address& addr) const
{
    auto [lspaces, rspaces] = trim_common(spaces_.spaces, addr.spaces_.spaces);
    if (lspaces.empty() && rspaces.empty())
        return address(merge_bases<merge_op::sub>(bases_, addr.bases_), offset_ - addr.offset_, {});

    normalization_helper helper;

    size_t l_processed = 0;

    if (rspaces.empty())
    {
        for (const auto& [sp, cnt] : lspaces)
        {
            if (cnt == 0 || sp->resolved()
                || !helper.map.try_emplace(sp.get(), l_processed).second) // requires normalization
                break;

            ++l_processed;
        }
        if (l_processed == lspaces.size())
            return address(merge_bases<merge_op::sub>(bases_, addr.bases_),
                offset_ - addr.offset_,
                space_list(lspaces, spaces_.owner));
    }

    auto res_spaces = std::make_shared<std::vector<space_entry>>();

    if (auto processed = lspaces.subspan(0, l_processed); !processed.empty())
    {
        res_spaces->insert(res_spaces->end(), processed.begin(), processed.end());
        lspaces = lspaces.subspan(l_processed);
    }

    int offset = 0;
    offset += get_unresolved_spaces(lspaces, helper, *res_spaces, 1);
    offset += get_unresolved_spaces(rspaces, helper, *res_spaces, -1);

    cleanup_spaces(*res_spaces);

    return address(merge_bases<merge_op::sub>(bases_, addr.bases_),
        offset_ - addr.offset_ + offset,
        space_list(std::move(res_spaces)));
}

address address::operator-(int offs) const { return address(bases_, offset_ - offs, spaces_); }

address address::operator-() const
{
    auto [spaces, off] = normalized_spaces();
    auto inv_bases = bases_;
    for (auto& b : inv_bases)
        b.second = -b.second;
    for (auto& s : spaces)
        s.second = -s.second;
    return address(std::move(inv_bases),
        -offset_ - off,
        space_list(std::make_shared<std::vector<space_entry>>(std::move(spaces))));
}

bool address::is_complex() const { return bases_.size() > 1; }

bool address::in_same_loctr(const address& addr) const
{
    if (!is_simple() || !addr.is_simple())
        return false;

    if (addr.bases_[0].first != bases_[0].first)
        return false;

    auto [spaces, _] = normalized_spaces();
    auto [addr_spaces, __] = addr.normalized_spaces();

    bool this_has_loctr_begin = spaces.size() && spaces[0].first->kind == space_kind::LOCTR_BEGIN;
    bool addr_has_loctr_begin = addr_spaces.size() && addr_spaces[0].first->kind == space_kind::LOCTR_BEGIN;

    if (this_has_loctr_begin && addr_has_loctr_begin)
        return spaces[0].first == addr_spaces[0].first;
    else if (!this_has_loctr_begin && !addr_has_loctr_begin)
        return true;
    else
    {
        if (spaces.size() && addr_spaces.size())
            return spaces.front().first->owner.name == addr_spaces.front().first->owner.name;
        return false;
    }
}

bool address::is_simple() const { return bases_.size() == 1 && bases_[0].second == 1; }

bool address::has_dependant_space() const
{
    if (!has_spaces() || spaces_.spaces.size() == 1 && spaces_.spaces.front().first->kind == space_kind::LOCTR_BEGIN)
        return false;
    auto [spaces, _] = normalized_spaces();
    if (spaces.empty() || spaces.size() == 1 && spaces.front().first->kind == space_kind::LOCTR_BEGIN)
        return false;
    return true;
}

bool address::has_unresolved_space() const
{
    if (!has_spaces())
        return false;

    auto [spaces, _] = normalized_spaces();

    return !spaces.empty();
}

bool address::has_spaces() const { return !spaces_.empty(); }

address::address(std::vector<base_entry> bases_, int offset_, space_list spaces)
    : bases_(std::move(bases_))
    , offset_(offset_)
    , spaces_(std::move(spaces))
{}

} // namespace hlasm_plugin::parser_library::context
