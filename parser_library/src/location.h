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

#include <string>
#include <vector>

#include "range.h"

namespace hlasm_plugin::parser_library {

struct PARSER_LIBRARY_EXPORT location
{
    location() {}
    location(position pos, std::string file)
        : pos(pos)
        , file(file)
    {}
    bool operator==(const location& oth) const { return pos == oth.pos && file == oth.file; }
    position pos;
    std::string file;
};

using location_list = std::vector<location>;

} // namespace hlasm_plugin::parser_library

#endif // !HLASMPLUGIN_PARSERLIBRARY_LOCATION_H
