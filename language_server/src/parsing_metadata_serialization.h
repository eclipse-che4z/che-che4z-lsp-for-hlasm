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

#include "workspace_manager.h"

namespace hlasm_plugin::parser_library {



void inline to_json(nlohmann::json& j, const parser_library::workspace_file_info& info)
{
    j = nlohmann::json { { "config_parsing", info.config_parsing },
        { "diagnostics_suppressed", info.diagnostics_suppressed },
        { "processor_group_found", info.processor_group_found } };
}

void inline to_json(nlohmann::json& j, const parser_library::performance_metrics& metrics)
{
    j = nlohmann::json { { "Open Code Statements", metrics.open_code_statements },
        { "Copy Statements", metrics.copy_statements },
        { "Macro Statements", metrics.macro_statements },
        { "Copy Def Statements", metrics.copy_def_statements },
        { "Macro Def Statements", metrics.macro_def_statements },
        { "Lookahead Statements", metrics.lookahead_statements },
        { "Reparsed Statements", metrics.reparsed_statements },
        { "Continued Statements", metrics.continued_statements },
        { "Non-continued Statements", metrics.non_continued_statements },
        { "Lines", metrics.lines },
        { "Files", metrics.files } };
}

void inline to_json(nlohmann::json& j, const parser_library::parsing_metadata& pm)
{
    j = nlohmann::json { { "metrics", pm.metrics }, { "ws_info", pm.ws_info } };
}


} // namespace hlasm_plugin::parser_library