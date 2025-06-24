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

#ifndef HLASMPLUGIN_UTILS_GENERAL_HASHERS_H
#define HLASMPLUGIN_UTILS_GENERAL_HASHERS_H

#include <string_view>

namespace hlasm_plugin::utils::hashers {
struct string_hasher
{
    using is_transparent = void;

    std::size_t operator()(std::string_view s) const
    {
        std::hash<std::string_view> hasher;
        return hasher(s);
    }
};

constexpr size_t hash_combine(std::size_t old, std::size_t next)
{
    return old ^ (next + 0x9e3779b9 + (old << 6) + (old >> 2));
}

} // namespace hlasm_plugin::utils::hashers

#endif
