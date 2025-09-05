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

#ifndef CONTEXT_ADDRESS_H
#define CONTEXT_ADDRESS_H

#include <compare>
#include <memory>
#include <span>
#include <utility>
#include <vector>

#include "alignment.h"
#include "context/id_index.h"

namespace hlasm_plugin::parser_library::context {

class location_counter;
class section;
struct space;
struct address;

using space_ptr = std::shared_ptr<space>;
using space_storage = std::vector<space_ptr>;
using aligned_addr = std::pair<address, space_ptr>;


// structure representing relative address in a section
struct address
{
    using space_entry = std::pair<space_ptr, int>;
    struct base_entry
    {
        id_index qualifier;
        const section* owner = nullptr;
        int32_t cardinality = 1;

        constexpr bool operator==(const base_entry&) const noexcept = default;
    };

    struct space_list
    {
        space_list() = default;
        template<typename T>
        explicit space_list(std::shared_ptr<T> ptr)
            : spaces(*ptr)
            , owner(std::move(ptr))
        {}
        explicit space_list(std::span<const space_entry> spaces, std::shared_ptr<const void> owner)
            : spaces(spaces)
            , owner(std::move(owner))
        {}

        std::span<const space_entry> spaces;
        std::shared_ptr<const void> owner;

        bool empty() const { return spaces.empty(); }
    };

    struct base_list
    {
        base_list() = default;
        template<typename T>
        explicit base_list(std::shared_ptr<T> ptr) requires(!std::same_as<T, base_entry>)
            : bases(*ptr)
            , owner(std::move(ptr))
        {}
        explicit base_list(std::shared_ptr<const base_entry> ptr)
            : bases(std::span(ptr.get(), !!ptr.get()))
            , owner(std::move(ptr))
        {}
        explicit base_list(std::span<const base_entry> bases, std::shared_ptr<const void> owner)
            : bases(bases)
            , owner(std::move(owner))
        {}

        std::span<const base_entry> bases;
        std::shared_ptr<const void> owner;

        bool empty() const { return bases.empty(); }
    };

private:
    // list of bases and their counts to which is the address relative
    base_list bases_;
    // offset relative to bases
    int offset_ = 0;
    // list of spaces with their counts this address contains
    space_list spaces_;

public:
    // list of bases and their counts to which is the address relative
    std::span<const base_entry> bases() const;
    // offset relative to bases
    int offset() const;
    int unresolved_offset() const;
    // list of spaces with their counts this address contains
    static std::pair<std::vector<space_entry>, int> normalized_spaces(std::span<const space_entry> spaces);
    std::pair<std::vector<space_entry>, int> normalized_spaces() const;

    address() = default;
    address(base_entry address_base, int offset, const space_storage& spaces);
    address(base_entry address_base, int offset, space_storage&& spaces);

    address operator+(const address& addr) const;
    address operator+(int offs) const;
    address operator-(const address& addr) const;
    address operator-(int offs) const;
    address operator-() const;

    bool is_complex() const;
    bool in_same_loctr(const address& addr) const;
    bool is_simple() const;
    bool has_dependant_space() const;
    bool has_unresolved_space() const;
    bool has_spaces() const;

    address with_base_list(base_list bl) const&;
    address with_base_list(base_list bl) &&;

private:
    address(base_list bases, int offset, space_list spaces);

    friend struct address_resolver;
    friend class location_counter;
};

enum class space_kind
{
    ORDINARY = 'O',
    LOCTR_BEGIN = 'B',
    ALIGNMENT = 'A',
    LOCTR_SET = 'S',
    LOCTR_MAX = 'M',
    LOCTR_UNKNOWN = 'U'
};

enum class resolve_reason : bool
{
    normal,
    cycle_removal,
};

// stucture representing space of unknown lenght in an adress structure (these are created when there are location
// counter dependencies in a code)
struct space
{
    const space_kind kind;
    // alignment of space end address
    alignment align;

    // previous address values
    // for LOCTR_UNKNOWN space kind
    size_t previous_boundary = 0;
    int previous_offset = 0;

    location_counter& owner;

    space(location_counter& owner, alignment align, space_kind kind);
    space(location_counter& owner, alignment align, size_t boundary, int offset);

    space(const space&) = delete;

    // fill space with a length
    void resolve(int length, resolve_reason r);
    // replace space with another
    void resolve(space_ptr value);
    // fill space with the whole address
    void resolve(int length, std::vector<address::space_entry> unresolved);

    bool resolved() const { return resolved_; }
    int resolved_length;
    std::vector<address::space_entry> resolved_ptrs;

private:
    bool resolved_;
};

} // namespace hlasm_plugin::parser_library::context

#endif
