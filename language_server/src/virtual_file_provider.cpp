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

#include "virtual_file_provider.h"

#include "logger.h"
#include "nlohmann/json.hpp"
#include "workspace_manager.h"

namespace {
std::string_view extract_method(const nlohmann::json& msg)
{
    auto method_it = msg.find("method");
    if (method_it == msg.end() || !method_it->is_string())
        return {};
    return method_it->get<std::string_view>();
}
} // namespace

namespace hlasm_plugin::language_server {

void virtual_file_provider::write(const nlohmann::json& m)
{
    try
    {
        std::string s(ws_mngr->get_virtual_file_content(m.at("params").at("id").get<unsigned long long>()));
        if (s.empty())
            out_stream->write(nlohmann::json {
                { "id", m.at("id") },
                {
                    "error",
                    nlohmann::json {
                        { "code", 1 },
                        { "message", "File not found" },
                    },
                },
            });
        else
            out_stream->write(nlohmann::json {
                { "id", m.at("id") },
                {
                    "result",
                    nlohmann::json {
                        { "content", std::move(s) },
                    },
                },
            });
    }
    catch (const nlohmann::json::exception& e)
    {
        LOG_ERROR(std::string("Virtual file provider exception: ") + e.what());
    }
}
void virtual_file_provider::write(nlohmann::json&& m) { write(m); }

message_router::message_predicate virtual_file_provider::get_filtering_predicate() const
{
    return [](const nlohmann::json& msg) { return extract_method(msg) == "get_file_content"; };
}

} // namespace hlasm_plugin::language_server
