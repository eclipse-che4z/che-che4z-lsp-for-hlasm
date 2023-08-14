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

#ifndef CONTEXT_LITERAL_STORAGE_H
#define CONTEXT_LITERAL_STORAGE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <compare>
#include <cstring>
#include <optional>
#include <string>
#include <unordered_set>

#include "id_index.h"

namespace hlasm_plugin::parser_library::context {
// storage for identifiers
// changes strings of identifiers to indexes of this storage class for easier and unified work
class id_storage
{
    std::unordered_set<std::string> lit_;

    static id_index small_id(std::string_view value);

public:
    size_t size() const;
    bool empty() const;

    std::optional<id_index> find(std::string_view val) const;

    id_index add(std::string_view value);
    id_index add(std::string&& value);

    struct well_known
    {
        static constexpr id_index COPY = id_index("COPY");
        static constexpr id_index SETA = id_index("SETA");
        static constexpr id_index SETB = id_index("SETB");
        static constexpr id_index SETC = id_index("SETC");
        static constexpr id_index GBLA = id_index("GBLA");
        static constexpr id_index GBLB = id_index("GBLB");
        static constexpr id_index GBLC = id_index("GBLC");
        static constexpr id_index LCLA = id_index("LCLA");
        static constexpr id_index LCLB = id_index("LCLB");
        static constexpr id_index LCLC = id_index("LCLC");
        static constexpr id_index MACRO = id_index("MACRO");
        static constexpr id_index MEND = id_index("MEND");
        static constexpr id_index MEXIT = id_index("MEXIT");
        static constexpr id_index MHELP = id_index("MHELP");
        static constexpr id_index ASPACE = id_index("ASPACE");
        static constexpr id_index AIF = id_index("AIF");
        static constexpr id_index AIFB = id_index("AIFB");
        static constexpr id_index AGO = id_index("AGO");
        static constexpr id_index AGOB = id_index("AGOB");
        static constexpr id_index ACTR = id_index("ACTR");
        static constexpr id_index AREAD = id_index("AREAD");
        static constexpr id_index ALIAS = id_index("ALIAS");
        static constexpr id_index END = id_index("END");
        static constexpr id_index SYSLIST = id_index("SYSLIST");
        static constexpr id_index ANOP = id_index("ANOP");
        static constexpr id_index AEJECT = id_index("AEJECT");
    };
};

} // namespace hlasm_plugin::parser_library::context

#endif
