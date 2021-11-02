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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DIAGNOSTIC_COUNTER_H
#define HLASMPLUGIN_LANGUAGESERVER_DIAGNOSTIC_COUNTER_H

#include "nlohmann/json.hpp"
#include "workspace_manager.h"

namespace hlasm_plugin::benchmark {

class diagnostic_counter : public hlasm_plugin::parser_library::diagnostics_consumer
{
public:
    void consume_diagnostics(hlasm_plugin::parser_library::diagnostic_list diagnostics) override
    {
        for (size_t i = 0; i < diagnostics.diagnostics_size(); i++)
        {
            if (auto diag_sev = diagnostics.diagnostics(i).severity();
                diag_sev == hlasm_plugin::parser_library::diagnostic_severity::error)
                error_count++;
            else if (diag_sev == hlasm_plugin::parser_library::diagnostic_severity::warning)
                warning_count++;
            message_counts[diagnostics.diagnostics(i).code()]++;
        }
    }

    void clear_counters()
    {
        error_count = 0;
        warning_count = 0;
        message_counts.clear();
    }

    size_t error_count = 0;
    size_t warning_count = 0;

    std::unordered_map<std::string, unsigned> message_counts;
};

inline nlohmann::json get_top_messages(const std::unordered_map<std::string, unsigned>& msgs, size_t limit = 3)
{
    std::vector<std::pair<std::string, unsigned>> top_msgs(limit);

    constexpr const auto cmp_msg = [](const auto& a, const auto& b) { return a.second > b.second; };

    const auto last = std::partial_sort_copy(msgs.begin(), msgs.end(), top_msgs.begin(), top_msgs.end(), cmp_msg);
    top_msgs.erase(last, top_msgs.end());

    nlohmann::json result = nlohmann::json::object();
    for (auto&& [key, value] : top_msgs)
        result[std::move(key)] = value;
    return result;
}


} // namespace hlasm_plugin::benchmark



#endif