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

#ifndef HLASMPLUGIN_LANGUAGESERVER_PARSING_METADATA_SINK_H
#define HLASMPLUGIN_LANGUAGESERVER_PARSING_METADATA_SINK_H

#include "workspace_manager.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server {

class parsing_metadata_collector : public parser_library::parsing_metadata_consumer
{
    void consume_parsing_metadata(const parser_library::parsing_metadata& metadata) override { data = metadata; }

    parser_library::parsing_metadata data;
};

inline nlohmann::json get_metrics_json(const parser_library::performance_metrics& metrics)
{
    return json {
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
        { "Files", metrics.files }
    };
}

} // namespace hlasm_plugin::language_server



#endif