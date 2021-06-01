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

#ifndef HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H
#define HLASMPLUGIN_LANGUAGESERVER_FEATURE_TEXTSYNCHRONIZATION_H

#include <vector>

#include "../feature.h"
#include "workspace_manager.h"


namespace hlasm_plugin::language_server::lsp {

// Groups text synchronization methods, which decode incomming notifications and call methods of
// provided workspace_manager. Also consumes highligting from library and sends it as json to LSP client.
class feature_text_synchronization : public feature
{
public:
    // synchronization kind from LSP specification, we use incremental
    enum text_document_sync_kind
    {
        none = 0,
        full = 1,
        incremental = 2
    };

    // Constructs the feature with underlying workspace_manager and response_provider to send messages to LSP client.
    feature_text_synchronization(parser_library::workspace_manager& ws_mngr, response_provider& response_provider);

    // Adds the implemented methods into the map.
    void register_methods(std::map<std::string, method>& methods) override;
    // Returns set capabilities connected with text synchonization
    json register_capabilities() override;
    // Does nothing, not needed.
    void initialize_feature(const json& initialise_params) override;

private:
    // Handles textDocument/didOpen notification.
    void on_did_open(const json& id, const json& params);
    // Handles textDocument/didChange notification.
    void on_did_change(const json& id, const json& params);
    // Handles textDocument/didClose notification.
    void on_did_close(const json& id, const json& params);
};

} // namespace hlasm_plugin::language_server::lsp

#endif
