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

#include "../logger.h"
#include "lib_config.h"

namespace hlasm_plugin::language_server::lsp {

feature_workspace_folders::feature_workspace_folders(
    parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : feature(ws_mngr, response_provider)
{}

void feature_workspace_folders::register_methods(std::map<std::string, method>& methods)
{
    methods.emplace("workspace/didChangeWorkspaceFolders",
        std::bind(&feature_workspace_folders::on_did_change_workspace_folders,
            this,
            std::placeholders::_1,
            std::placeholders::_2));
    methods.emplace("workspace/didChangeWatchedFiles",
        std::bind(
            &feature_workspace_folders::did_change_watched_files, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace("workspace/didChangeConfiguration",
        std::bind(
            &feature_workspace_folders::did_change_configuration, this, std::placeholders::_1, std::placeholders::_2));
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
        add_workspace(uri, uri_to_path(uri));
        return;
    }

    auto root_path = initialize_params.find("rootPath");
    if (root_path != initialize_params.end() && !root_path->is_null())
    {
        std::filesystem::path path(root_path->get<std::string>());
        add_workspace(path.lexically_normal().string(), path.lexically_normal().string());
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

        add_workspace(name, uri_to_path(uri));
    }
}
void feature_workspace_folders::remove_workspaces(const json& removed)
{
    for (auto ws : removed)
    {
        std::string uri = ws["uri"].get<std::string>();

        ws_mngr_.remove_workspace(uri_to_path(uri).c_str());
    }
}
void feature_workspace_folders::add_workspace(const std::string& name, const std::string& path)
{
    ws_mngr_.add_workspace(name.c_str(), path.c_str());
}



void feature_workspace_folders::did_change_watched_files(const json&, const json& params)
{
    std::vector<json> changes = params["changes"];
    std::vector<std::string> paths;
    for (auto& change : changes)
        paths.push_back(uri_to_path(change["uri"].get<std::string>()));
    std::vector<const char*> c_uris;
    std::transform(
        paths.begin(), paths.end(), std::back_inserter(c_uris), [](const std::string& s) { return s.c_str(); });
    ws_mngr_.did_change_watched_files(c_uris.data(), c_uris.size());
}

void feature_workspace_folders::send_configuration_request()
{
    static const json config_request_args { { "items", { { { "section", "hlasm" } } } } };
    response_->request("config_request_" + std::to_string(config_request_number_),
        "workspace/configuration",
        config_request_args,
        std::bind(&feature_workspace_folders::configuration, this, std::placeholders::_1, std::placeholders::_2));
    ++config_request_number_;
}

void feature_workspace_folders::configuration(const json&, const json& params) const
{
    if (params.size() == 0)
    {
        LOG_WARNING("Empty configuration response received.");
        return;
    }

    lib_config::load_from_json(params[0]);
}

void feature_workspace_folders::did_change_configuration(const json&, const json&) { send_configuration_request(); }

} // namespace hlasm_plugin::language_server::lsp
