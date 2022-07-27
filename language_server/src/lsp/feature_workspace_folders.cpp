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

#include "feature_workspace_folders.h"

#include <filesystem>

#include "network/uri/uri.hpp"

#include "../logger.h"
#include "lib_config.h"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"

namespace hlasm_plugin::language_server::lsp {

feature_workspace_folders::feature_workspace_folders(
    parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : feature(ws_mngr, response_provider)
{}

void feature_workspace_folders::register_methods(std::map<std::string, method>& methods)
{
    methods.try_emplace("workspace/didChangeWorkspaceFolders",
        method { [this](const json& id, const json& args) { on_did_change_workspace_folders(id, args); },
            telemetry_log_level::LOG_EVENT });
    methods.try_emplace("workspace/didChangeWatchedFiles",
        method { [this](const json& id, const json& args) { did_change_watched_files(id, args); },
            telemetry_log_level::NO_TELEMETRY });
    methods.try_emplace("workspace/didChangeConfiguration",
        method { [this](const json& id, const json& args) { did_change_configuration(id, args); },
            telemetry_log_level::NO_TELEMETRY });
}

json feature_workspace_folders::register_capabilities()
{
    return json { { "workspace",
        json { { "workspaceFolders", json { { "supported", true }, { "changeNotifications", true } } } } } };
}

void feature_workspace_folders::initialize_feature(const json& initialize_params)
{
    // Get config at initialization
    send_configuration_request();

    bool ws_folders_support = false;
    auto capabs = initialize_params["capabilities"];
    auto ws = capabs.find("workspace");
    if (ws != capabs.end())
    {
        auto ws_folders = ws->find("workspaceFolders");
        if (ws_folders != ws->end())
            ws_folders_support = ws_folders->get<bool>();
    }

    if (ws_folders_support)
    {
        add_workspaces(initialize_params["workspaceFolders"]);
        return;
    }


    auto root_uri = initialize_params.find("rootUri");
    if (root_uri != initialize_params.end() && !root_uri->is_null())
    {
        std::string uri = root_uri->get<std::string>();
        add_workspace(uri, uri);
        return;
    }

    auto root_path = initialize_params.find("rootPath");
    if (root_path != initialize_params.end() && !root_path->is_null())
    {
        auto uri = utils::path::path_to_uri(utils::path::lexically_normal(root_path->get<std::string>()).string());
        add_workspace(uri, uri);
    }
}


void feature_workspace_folders::on_did_change_workspace_folders(const json&, const json& params)
{
    const auto& added = params["event"]["added"];
    const auto& removed = params["event"]["removed"];

    remove_workspaces(removed);
    add_workspaces(added);
}

void feature_workspace_folders::add_workspaces(const json& added)
{
    for (auto& it : added)
    {
        const std::string& name = it["name"].get<std::string>();
        std::string uri = it["uri"].get<std::string>();

        add_workspace(name, uri);
    }
}
void feature_workspace_folders::remove_workspaces(const json& removed)
{
    for (auto ws : removed)
    {
        std::string uri = ws["uri"].get<std::string>();

        ws_mngr_.remove_workspace(uri.c_str());
    }
}
void feature_workspace_folders::add_workspace(const std::string& name, const std::string& path)
{
    ws_mngr_.add_workspace(name.c_str(), path.c_str());
}

void feature_workspace_folders::did_change_watched_files(const json&, const json& params)
{
    using namespace hlasm_plugin::parser_library;
    try
    {
        const auto& json_changes = params.at("changes");
        std::vector<fs_change> changes;
        changes.reserve(json_changes.size());
        std::transform(json_changes.begin(), json_changes.end(), std::back_inserter(changes), [](const json& change) {
            auto uri = change.at("uri").get<std::string_view>();
            auto type = change.at("type").get<long long>();
            if (type < 1 || type > 3)
                type = 0;
            return fs_change { sequence<char>(uri), static_cast<fs_change_type>(type) };
        });
        ws_mngr_.did_change_watched_files(sequence<fs_change>(changes));
    }
    catch (const nlohmann::json::exception& j)
    {
        LOG_ERROR(std::string("Invalid didChangeWatchedFiles notification parameter: ") + j.what());
    }
}

void feature_workspace_folders::send_configuration_request()
{
    static const json config_request_args { {
        "items",
        json::array_t {
            { { "section", "hlasm" } },
            json::object(),
        },
    } };
    response_->request("config_request_" + std::to_string(config_request_number_),
        "workspace/configuration",
        config_request_args,
        { [this](const json& id, const json& params) { configuration(id, params); }, telemetry_log_level::LOG_EVENT });
    ++config_request_number_;
}

void feature_workspace_folders::configuration(const json&, const json& params) const
{
    if (params.size() != 2)
    {
        LOG_WARNING("Unexpected configuration response received.");
        return;
    }

    ws_mngr_.configuration_changed(parser_library::lib_config::load_from_json(params[0]), params[1].dump().c_str());
}

void feature_workspace_folders::did_change_configuration(const json&, const json&) { send_configuration_request(); }

} // namespace hlasm_plugin::language_server::lsp
