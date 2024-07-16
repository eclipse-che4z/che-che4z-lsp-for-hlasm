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

#include "section.h"

#include <algorithm>
#include <memory>

#include "location_counter.h"

using namespace hlasm_plugin::parser_library::context;

const std::vector<std::unique_ptr<location_counter>>& section::location_counters() const { return loctrs_; }

section::section(id_index name, section_kind kind)
    : name(name)
    , kind(kind)
{
    loctrs_.emplace_back(std::make_unique<location_counter>(name, *this, loctr_kind::STARTING));
    curr_loctr_ = loctrs_.back().get();
}

section::section(id_index name, section_kind kind, goff_details details)
    : name(name)
    , kind(kind)
    , goff(std::move(details))
{
    loctrs_.emplace_back(std::make_unique<location_counter>(name, *this, loctr_kind::STARTING));
    curr_loctr_ = loctrs_.back().get();
}

location_counter& section::set_location_counter(id_index loctr_name)
{
    auto tmp = std::ranges::find(loctrs_, loctr_name, &location_counter::name);

    if (tmp != loctrs_.end())
        curr_loctr_ = std::to_address(*tmp);
    else
    {
        loctrs_.emplace_back(std::make_unique<location_counter>(loctr_name, *this, loctr_kind::NONSTARTING));
        curr_loctr_ = loctrs_.back().get();
    }
    return *curr_loctr_;
}

location_counter& section::set_location_counter(location_counter& l)
{
    assert(std::ranges::find(loctrs_, &l, &std::unique_ptr<location_counter>::get) != loctrs_.end());
    curr_loctr_ = &l;
    return l;
}

location_counter* section::find_location_counter(id_index loctr_name)
{
    if (auto it = std::ranges::find(loctrs_, loctr_name, &location_counter::name); it != loctrs_.end())
        return it->get();
    else
        return nullptr;
}

location_counter& section::current_location_counter() const { return *curr_loctr_; }
