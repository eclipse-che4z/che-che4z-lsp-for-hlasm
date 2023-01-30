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

#include "nlohmann/json_fwd.hpp"
#include "telemetry_info.h"
#include "workspace_manager.h"

namespace hlasm_plugin::parser_library {

void to_json(nlohmann::json& j, const parser_library::workspace_file_info& info);

void to_json(nlohmann::json& j, const parser_library::performance_metrics& metrics);

void to_json(nlohmann::json& j, const parser_library::parsing_metadata& metadata);

} // namespace hlasm_plugin::parser_library


namespace hlasm_plugin::language_server {

void to_json(nlohmann::json& j, const telemetry_metrics_info& metrics);

void to_json(nlohmann::json& j, const telemetry_info& info);

void to_json(nlohmann::json& j, const telemetry_error& err);

void to_json(nlohmann::json& j, const telemetry_message& message);


} // namespace hlasm_plugin::language_server
