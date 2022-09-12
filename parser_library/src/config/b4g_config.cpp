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

#include "b4g_config.h"

#include "nlohmann/json.hpp"

namespace hlasm_plugin::parser_library::config {
void from_json(const nlohmann::json& j, b4g_map& p)
{
    auto ext = j.at("fileExtension").get<std::string>();
    if (!ext.empty())
        ext.insert(0, 1, '.');

    p.default_processor_group_name = j.at("defaultProcessorGroup").get<std::string>();
    p.files.clear();
    for (const auto& [key, value] : j.at("elements").items())
        p.files.try_emplace(key + ext, b4g_detail { value.at("processorGroup").get<std::string>() });
}
} // namespace hlasm_plugin::parser_library::config
