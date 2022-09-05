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
#include <utility>
#include <variant>
#include <vector>

#include "alignment.h"
#include "context/id_storage.h"

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
    struct base
    {
        const section* owner = nullptr;
        id_index qualifier = nullptr;

        friend bool operator==(const base&, const base&) = default;
    };

    using space_entry = std::pair<space_ptr, int>;
    using base_entry = std::pair<base, int>;

private:
    // list of bases and their counts to which is the address relative
    std::vector<base_entry> bases_;
    // offset relative to bases
    int offset_ = 0;
    // list of spaces with their counts this address contains
    std::vector<space_entry> spaces_;

public:
    // list of bases and their counts to which is the address relative
    const std::vector<base_entry>& bases() const;
    std::vector<base_entry>& bases();
    // offset relative to bases
    int offset() const;
    // list of spaces with their counts this address contains
    std::vector<space_entry>& spaces();
    const std::vector<space_entry>& spaces() const;
    std::vector<space_entry> normalized_spaces() const;

    address() = default;
    address(base address_base, int offset, const space_storage& spaces);

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

    void normalize();

private:
    address(std::vector<base_entry> bases, int offset, std::vector<space_entry> spaces);
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
    static void resolve(space_ptr this_space, int length);
    // replace space with another
    static void resolve(space_ptr this_space, space_ptr value);
    // fill space with the whole address
    static void resolve(space_ptr this_space, address value);
    // common resolver for 2 methods above
    static void resolve(space_ptr this_space, std::variant<space_ptr, address> value);

    bool resolved() const { return resolved_; }
    int resolved_length;
    std::vector<address::space_entry> resolved_ptrs;

private:
    bool resolved_;
};

} // namespace hlasm_plugin::parser_library::context

#endif
