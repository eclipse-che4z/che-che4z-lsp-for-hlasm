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

#include "dap_session_manager.h"

#include "dap_message_wrappers.h"
#include "dap_session.h"
#include "nlohmann/json.hpp"
#include "telemetry_sink.h"

namespace {
std::optional<size_t> extract_session_id_from_registration_message(const nlohmann::json& msg)
{
    const static std::string session_id_key_name = "session_id";
    auto params = msg.find("params");
    if (params == msg.end())
        return std::nullopt;
    auto session_id = params->find(session_id_key_name);
    if (session_id == params->end() || !session_id->is_number())
        return std::nullopt;
    return session_id->get<size_t>();
}
std::string_view extract_method(const nlohmann::json& msg)
{
    auto method_it = msg.find("method");
    if (method_it == msg.end() || !method_it->is_string())
        return {};
    return method_it->get<std::string_view>();
}
} // namespace

namespace hlasm_plugin::language_server::dap {
void session_manager::cleanup_sessions()
{
    for (auto it = sessions.begin(); it != sessions.end();)
    {
        if (it->second->is_running())
            ++it;
        else
            it = sessions.erase(it);
    }
}
void session_manager::handle_registration_request(size_t new_id)
{
    cleanup_sessions();

    // currently only one debug session is supported
    auto new_session = std::make_unique<dap::session>(new_id, *ws_mngr, *out_stream, telemetry_reporter, ext_files);
    sessions.try_emplace(new_session->get_session_id(), std::move(new_session));
}
session_manager::session_manager(hlasm_plugin::parser_library::workspace_manager& ws,
    json_sink& out,
    telemetry_sink* telem_reporter,
    external_file_reader* ext_files)
    : ws_mngr(&ws)
    , out_stream(&out)
    , telemetry_reporter(telem_reporter)
    , ext_files(ext_files)
{}
session_manager::session_manager(session_manager&&) noexcept = default;
session_manager& session_manager::operator=(session_manager&&) noexcept = default;
session_manager::~session_manager() = default;

// Inherited via json_sink

void session_manager::write(const nlohmann::json& msg)
{
    auto method = extract_method(msg);
    if (auto it = sessions.find(method); it != sessions.end())
    {
        it->second->write(msg);
    }
    else if (method == hlasm_plugin::language_server::dap::broadcom_tunnel_method)
    {
        if (auto new_id = extract_session_id_from_registration_message(msg); new_id.has_value())
            handle_registration_request(new_id.value());
    }
}
void session_manager::write(nlohmann::json&& msg)
{
    auto method = extract_method(msg);
    if (auto it = sessions.find(method); it != sessions.end())
    {
        it->second->write(std::move(msg));
    }
    else if (method == hlasm_plugin::language_server::dap::broadcom_tunnel_method)
    {
        if (auto new_id = extract_session_id_from_registration_message(msg); new_id.has_value())
            handle_registration_request(new_id.value());
    }
}
message_router::message_predicate session_manager::get_filtering_predicate() const
{
    return [](const nlohmann::json& msg) {
        auto method = extract_method(msg);
        return method.substr(0, dap::broadcom_tunnel_method.size()) == dap::broadcom_tunnel_method;
    };
}
} // namespace hlasm_plugin::language_server::dap
