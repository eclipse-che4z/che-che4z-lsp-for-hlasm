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

#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_WORKSPACEFOLDERS_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_WORKSPACEFOLDERS_H

#include <vector>

#include "../feature.h"
#include "workspace_manager.h"


namespace hlasm_plugin::language_server::lsp {

// Groups workspace methods, which decode incomming notifications and call methods of
// provided workspace_manager.
class feature_workspace_folders : public feature
{
public:
    explicit feature_workspace_folders(
        parser_library::workspace_manager& ws_mngr, response_provider& response_provider);

    // Adds workspace/* methods to the map.
    void register_methods(std::map<std::string, method>&) override;
    // Returns workspaces capability.
    nlohmann::json register_capabilities() override;
    // Opens workspace specified in the initialize request.
    void initialize_feature(const nlohmann::json& initialise_params) override;
    void initialized() override;

private:
    // Handles workspace/didChangeWorkspaceFolders notification.
    void on_did_change_workspace_folders(const nlohmann::json& params);
    void did_change_watched_files(const nlohmann::json& params);

    void configuration(const nlohmann::json& params) const;
    void did_change_configuration(const nlohmann::json& params);


    // Adds all workspaces specified in the json to the workspace manager.
    void add_workspaces(const nlohmann::json& added);

    // Removes all workspaces specified in the json from the workspace manager.
    void remove_workspaces(const nlohmann::json& removed);

    // Adds one workspace to the workspace manager.
    void add_workspace(const std::string& name, const std::string& uri);

    void send_configuration_request();

    parser_library::workspace_manager& ws_mngr_;
    std::vector<std::pair<std::string, std::string>> m_initial_workspaces;
};

} // namespace hlasm_plugin::language_server::lsp


#endif
