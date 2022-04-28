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

#ifndef HLASMPARSER_PARSERLIBRARY_CONFIG_ASSEMBLER_OPTIONS_H
#define HLASMPARSER_PARSERLIBRARY_CONFIG_ASSEMBLER_OPTIONS_H

#include <optional>
#include <string>

#include "nlohmann/json_fwd.hpp"

namespace hlasm_plugin::parser_library {
struct asm_option;
} // namespace hlasm_plugin::parser_library

namespace hlasm_plugin::parser_library::config {

struct assembler_options
{
    std::optional<std::string> sysparm;
    std::optional<std::string> profile;
    std::optional<std::string> optable;
    std::optional<std::string> system_id;
    std::optional<bool> goff;

    bool operator==(const assembler_options&) const = default;
    bool valid() const noexcept;
    void apply(asm_option& opts) const;
    bool has_value() const noexcept;
};
void to_json(nlohmann::json& j, const assembler_options& p);
void from_json(const nlohmann::json& j, assembler_options& p);
} // namespace hlasm_plugin::parser_library::config
#endif
