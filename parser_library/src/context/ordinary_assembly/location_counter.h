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

#ifndef CONTEXT_LOCATION_COUNTER_H
#define CONTEXT_LOCATION_COUNTER_H

#include "location_counter_data.h"

namespace hlasm_plugin::parser_library::context {

class section;
class location_counter;

using loctr_ptr = std::unique_ptr<location_counter>;

// enum stating whether the location counter is the first in section or not
enum class loctr_kind
{
    STARTING,
    NONSTARTING
};

// class representing section's location counter
class location_counter
{
    // active ORG data
    std::vector<location_counter_data> org_data_;
    // helper vector for switching to unresolved ORG data
    std::vector<location_counter_data> switched_org_data_;
    // identifier space of switched unresolved ORG data
    space_ptr switched_;

    size_t last_space_;
    id_storage& ids_;

    bool layuot_created_;

public:
    const id_index name;
    const section& owner;
    const loctr_kind kind;

    bool has_unresolved_spaces() const;
    size_t storage() const;

    location_counter(id_index name, const section& owner, const loctr_kind kind, id_storage& ids);

    address current_address();

    // reserves storage area of specified length and alignment
    aligned_addr reserve_storage_area(size_t length, alignment a);

    // aligns storage
    aligned_addr align(alignment align);

    // adds space to the top of the storage
    space_ptr register_ordinary_space(alignment align);

    bool need_space_alignment(alignment align) const;

    // sets value of the location counter to the specified address (ORG instruction)
    space_ptr set_value(const address& addr, size_t boundary, int offset, bool has_undefined_part);

    // sets the location counter to the next available location (ORG with empty first param)
    std::pair<space_ptr, std::vector<address>> set_available_value();

    // creates layout
    void finish_layout(size_t offset);

    void resolve_space(space_ptr sp, int length);

    // switches to unresolved ORG identified by space
    void switch_to_unresolved_value(space_ptr sp);
    // restores to the current ORG data
    std::variant<space_ptr, address> restore_from_unresolved_value(space_ptr sp);

    bool check_underflow();

private:
    context::id_index create_space_name(char type);

    space_ptr register_space(alignment align, space_kind kind);
    space_ptr register_space(alignment align, address addr, size_t boundary, int offset);

    bool has_alignment(alignment align) const;

    // adds alignment requirement for the top of the storage
    space_ptr register_align_space(alignment align);

    location_counter_data& curr_data();
    const location_counter_data& curr_data() const;

    void check_available_value();
    bool check_if_higher_value(size_t idx) const;
};

} // namespace hlasm_plugin::parser_library::context
#endif
