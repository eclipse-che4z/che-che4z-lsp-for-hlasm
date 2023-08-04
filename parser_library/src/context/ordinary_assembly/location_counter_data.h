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

#ifndef CONTEXT_LOCATION_COUNTER_DATA_H
#define CONTEXT_LOCATION_COUNTER_DATA_H

#include <list>
#include <memory>
#include <optional>

#include "address.h"
#include "alignment.h"

namespace hlasm_plugin::parser_library::context {

/*
   to satisfy the requirements of ORG instructions, multuple location counter data are needed

   suppose that current location counter value equals to 11+X that came from this instructions:
     DS 11C
   A DS (X)C    represented by column 1

   then we use
     ORG A
     DS  (Y)C   represented by column 2

   last we use
     ORG A,,-3
     DS  (Z)C   represented by column 3

   symbols X,Y,Z were not previously defined when used in DS statements
   so we do not know current location counter value

   therefore when we use instruction
    ORG ,       represented by column 4
   we need to compute max of stored previous location counter values to correctly set the value

      1         2         3           4
   +-----+   +-----+   +-----+  |  +-----+
   |     |   |     |   |  8  |  |  |     |
   | 11  |   | 11  |   |-----|  |  | 11  |
   A-----|   |-----|   |     |  |  |---- |
   |     |   |     |   |  Z  |  |  |     |
   |  X  |   |     |   +-----+  |  |     |
   |     |   |  Y  |            |  |  Y  |
   +-----+   |     |            |  |     |
             |     |            |  |     |
             +-----+            |  +-----+
   =======================================> ORG ,

   from implementation point of view, each column is represented by one location_counter_data object all stored in
   vector of the owning location_counter object any time an involved symbol is defined, the data is updated and the
   vector is reduced accordingly (i.e. when we know, that data will not be a competition in finding maximum)
*/



// states whether data is potential candidate for greatest storage in the loctr or it is not known yet
enum class loctr_data_kind
{
    POTENTIAL_MAX,
    UNKNOWN_MAX
};

// helper class for space and storage following it
struct space_storage_t
{
    space_ptr unknown_space;
    int storage_after;
};

// data of location counter for the active ORG
struct location_counter_data
{
    // count of virtually allocated bytes pior to this location counter
    int storage;
    // spaces with storages between them assigned to the counter
    std::list<space_storage_t> unknown_parts;
    int initial_storage;
    // number of bytes before end of the storage after last space (if any)
    int current_safe_area;
    loctr_data_kind kind;

    mutable std::shared_ptr<std::vector<address::space_entry>> cached_spaces_for_address;
    mutable std::optional<address::space_list> cached_pseudo_relative_spaces_for_address;

    location_counter_data();
    location_counter_data(loctr_data_kind kind);

    void append_space(space_ptr sp);
    void append_storage(int st);
    void append_data(location_counter_data data);
    void resolve_space(const space* sp, size_t length);
    void resolve_space(const space* sp, space_ptr new_space);
    bool has_alignment(alignment align) const;
    bool align(alignment align);
    bool need_space_alignment(alignment align) const;

    int last_storage() const;
    bool matches_first_space(const space*) const;
    bool has_space() const;
    space_ptr fist_space() const;
    space_ptr last_space() const;
    address::space_list spaces_for_address() const;
    address::space_list pseudo_relative_spaces(space_ptr loctr_start) const;
};

} // namespace hlasm_plugin::parser_library::context
#endif
