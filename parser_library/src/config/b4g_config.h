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

#ifndef HLASMPARSER_PARSERLIBRARY_CONFIG_B4G_CONFIG_H
#define HLASMPARSER_PARSERLIBRARY_CONFIG_B4G_CONFIG_H

#include <string>
#include <unordered_map>

#include "nlohmann/json_fwd.hpp"
#include "utils/general_hashers.h"

namespace hlasm_plugin::parser_library::config {
struct b4g_detail
{
    std::string processor_group_name;
};
struct b4g_map
{
    std::unordered_map<std::string, b4g_detail, utils::hashers::string_hasher, std::equal_to<>> files;
    std::string default_processor_group_name;
};

void from_json(const nlohmann::json& j, b4g_map& p);

} // namespace hlasm_plugin::parser_library::config

#endif
