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
#include "shared/workspace_manager.h"


namespace hlasm_plugin::language_server::lsp {

class feature_workspace_folders : public feature
{
public:
	feature_workspace_folders(parser_library::workspace_manager & ws_mngr);

	void register_methods(std::map<std::string, method> &) override;
	json virtual register_capabilities() override;
	void virtual initialize_feature(const json & initialise_params) override;
	
private:
	void on_did_change_workspace_folders(const json & id, const json & params);
	void did_change_watched_files(const json&, const json& params);
	void add_workspaces(const json & added);
	void remove_workspaces(const json & removed);

	void add_workspace(const std::string & name, const std::string & uri);
};

}


#endif
