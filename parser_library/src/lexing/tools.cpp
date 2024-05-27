/*
 * Copyright (c) 2022 Broadcom.
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

#include "tools.h"

#include <algorithm>
#include <array>

namespace hlasm_plugin::parser_library::lexing {

bool is_valid_symbol_name(std::string_view s, bool extended_names_allowed)
{
    static constexpr const auto allowed_symbols = []() {
        std::array<bool, 256> result = {};
        for (unsigned char c = 'a'; c <= 'z'; ++c)
            result[c] = true;
        for (unsigned char c = 'A'; c <= 'Z'; ++c)
            result[c] = true;
        for (unsigned char c = '0'; c <= '9'; ++c)
            result[c] = true;
        result['@'] = true;
        result['#'] = true;
        result['$'] = true;
        result['_'] = true;
        return result;
    }();
    if (s.empty())
        return false;
    if (s.size() > 63 || !extended_names_allowed && s.size() > 8)
        return false;
    if (s.front() >= '0' && s.front() <= '9')
        return false;
    return std::ranges::all_of(s, [](unsigned char c) { return c < allowed_symbols.size() && allowed_symbols[c]; });
}

} // namespace hlasm_plugin::parser_library::lexing
