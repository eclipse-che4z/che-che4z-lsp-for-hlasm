/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_INFO_H
#define HLASMPLUGIN_LANGUAGESERVER_TELEMETRY_INFO_H

#include <optional>
#include <string>
#include <variant>

#include "protocol.h"

namespace hlasm_plugin::language_server {

struct telemetry_metrics_info
{
    parser_library::parsing_metadata metadata;
};

struct telemetry_info
{
    std::string method_name;
    double duration;
    std::optional<telemetry_metrics_info> metrics;
};

struct telemetry_error
{
    std::string error_type;
    std::string error_details;
};

using telemetry_message = std::variant<telemetry_info, telemetry_error>;

} // namespace hlasm_plugin::language_server

#endif
