/*
 * Copyright (c) 2023 Broadcom.
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

#include "parsing_metadata_serialization.h"

#include "nlohmann/json.hpp"

namespace hlasm_plugin::parser_library {

void to_json(nlohmann::json& j, const parser_library::workspace_file_info& info)
{
    j = nlohmann::json {
        { "config_parsing", info.config_parsing },
        { "diagnostics_suppressed", info.diagnostics_suppressed },
        { "processor_group_found", info.processor_group_found },
        { "files_processed", info.files_processed },
    };
}

void to_json(nlohmann::json& j, const parser_library::performance_metrics& metrics)
{
    j = nlohmann::json {
        { "Open Code Statements", metrics.open_code_statements },
        { "Copy Statements", metrics.copy_statements },
        { "Macro Statements", metrics.macro_statements },
        { "Copy Def Statements", metrics.copy_def_statements },
        { "Macro Def Statements", metrics.macro_def_statements },
        { "Lookahead Statements", metrics.lookahead_statements },
        { "Reparsed Statements", metrics.reparsed_statements },
        { "Continued Statements", metrics.continued_statements },
        { "Non-continued Statements", metrics.non_continued_statements },
        { "Lines", metrics.lines },
    };
}

void to_json(nlohmann::json& j, const parser_library::parsing_metadata& metadata)
{
    j = nlohmann::json { { "properties", metadata.ws_info }, { "measurements", metadata.metrics } };
    j["measurements"]["error_count"] = metadata.errors;
    j["measurements"]["warning_count"] = metadata.warnings;
}

} // namespace hlasm_plugin::parser_library


namespace hlasm_plugin::language_server {

void to_json(nlohmann::json& j, const telemetry_metrics_info& metrics) { j = metrics.metadata; }

void to_json(nlohmann::json& j, const telemetry_info& info)
{
    if (info.metrics.has_value())
        j = *info.metrics;
    else
        j["properties"] = nlohmann::json::value_t::null;

    j["measurements"]["duration"] = info.duration;
    j["method_name"] = info.method_name;
}

void to_json(nlohmann::json& j, const telemetry_error& err)
{
    nlohmann::json properties;
    if (err.error_details != "")
        properties["error_details"] = err.error_details;
    j = nlohmann::json {
        { "method_name", "server_error/" + err.error_type }, { "properties", properties }, { "measurements", {} }
    };
}

void to_json(nlohmann::json& j, const telemetry_message& message)
{
    if (std::holds_alternative<telemetry_error>(message))
        j = std::get<telemetry_error>(message);
    else
        j = std::get<telemetry_info>(message);
}


} // namespace hlasm_plugin::language_server
