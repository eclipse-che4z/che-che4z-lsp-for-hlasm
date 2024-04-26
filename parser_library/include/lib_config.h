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

#ifndef HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H
#define HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H

#include <cstdint>
#include <optional>

namespace hlasm_plugin::parser_library {

// Encapsulates user defined settings of library and individual workspaces
struct lib_config
{
    // Returns a lib_config instance that is a copy of this instance, but the missing settings are replaced with the
    // parameter second. If there are more missing settings after that step, they are filled with default values
    [[nodiscard]] lib_config fill_missing_settings(const lib_config& second) const;

    std::optional<int64_t> diag_supress_limit;

    bool operator==(const lib_config&) const noexcept = default;
};

} // namespace hlasm_plugin::parser_library

#endif // HLASMPLUGIN_PARSERLIBRARY_LIB_CONFIG_H
