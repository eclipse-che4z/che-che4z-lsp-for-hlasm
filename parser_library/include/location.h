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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LOCATION_H
#define HLASMPLUGIN_PARSERLIBRARY_LOCATION_H

#include <compare>

#include "range.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::parser_library {

struct location
{
    location() = default;
    location(position pos, utils::resource::resource_location file)
        : pos(pos)
        , resource_loc(std::move(file))
    {}

    auto operator<=>(const location& oth) const noexcept = default;

    position pos;
    utils::resource::resource_location resource_loc;
};

} // namespace hlasm_plugin::parser_library

#endif // !HLASMPLUGIN_PARSERLIBRARY_LOCATION_H
