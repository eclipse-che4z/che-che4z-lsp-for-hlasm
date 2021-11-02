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

#ifndef HLASMPLUGIN_LANGUAGESERVER_COMMON_TYPES_H
#define HLASMPLUGIN_LANGUAGESERVER_COMMON_TYPES_H

#include "nlohmann/json.hpp"
// Types that are used throughout the language server component
namespace hlasm_plugin::language_server {

using json = nlohmann::json;

enum class telemetry_log_level
{
    NO_TELEMETRY,
    LOG_EVENT,
    LOG_WITH_PARSE_DATA
};

struct method
{
    std::function<void(const json& id, const json& params)> handler;
    telemetry_log_level telemetry_level;
};

using send_message_callback = std::function<void(const json&)>;
} // namespace hlasm_plugin::language_server

#endif