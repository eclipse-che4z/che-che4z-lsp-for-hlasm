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

void section::set_location_counter(id_index loctr_name)
{
    auto tmp = std::ranges::find(loctrs_, loctr_name, &location_counter::name);

    if (tmp != loctrs_.end())
        curr_loctr_ = std::to_address(*tmp);
    else
    {
        loctrs_.emplace_back(std::make_unique<location_counter>(loctr_name, *this, loctr_kind::NONSTARTING));
        curr_loctr_ = loctrs_.back().get();
    }
}

bool section::counter_defined(id_index loctr_name)
{
    return std::ranges::find(loctrs_, loctr_name, &location_counter::name) != loctrs_.end();
}

location_counter& section::current_location_counter() const { return *curr_loctr_; }
