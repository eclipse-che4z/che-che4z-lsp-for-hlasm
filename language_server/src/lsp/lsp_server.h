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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_LSP_SERVER_H

#include <functional>
#include <memory>
#include <unordered_set>

#include "json.hpp"

#include "../common_types.h"
#include "../feature.h"
#include "../server.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::lsp {

enum class message_type
{
    MT_ERROR = 1,
    MT_WARNING = 2,
    MT_INFO = 3,
    MT_LOG = 4
};

// Implements LSP server (session-controlling methods like initialize and shutdown).
// Integrates 3 features: language features, text synchronization and workspace folders.
// Consumes diagnostics that come from the parser library and sends them to LSP client.
class server final : public hlasm_plugin::language_server::server, public parser_library::diagnostics_consumer
{
public:
    // Creates the server with workspace_manager as entry point to parser library.
    server(parser_library::workspace_manager& ws_mngr);

    // Parses LSP (JSON RPC) message and calls corresponding method.
    virtual void message_received(const json& message) override;

protected:
    // Sends respond to request to LSP client using send_message_provider.
    virtual void respond(const json& id, const std::string& requested_method, const json& args) override;
    // Sends notification to LSP client using send_message_provider.
    virtual void notify(const std::string& method, const json& args) override;
    // Sends errorous respond to LSP client using send_message_provider.
    virtual void respond_error(const json& id,
        const std::string& requested_method,
        int err_code,
        const std::string& err_message,
        const json& error) override;
private:
    // requests

    // Implements initialize request.
    void on_initialize(json id, const json& param);
    // Implements the LSP shutdown request.
    void on_shutdown(json id, const json& param);


    // notifications

    // Implements the LSP exit request.
    void on_exit(json id, const json& param);


    // client notifications

    // Implements the LSP showMessage request.
    void show_message(const std::string& message, message_type type);

    // Remembers name of files for which were sent diagnostics the last time
    // diagnostics were sent to client. Used to clear diagnostics in client
    // when no more diags are produced by server for particular file.
    std::unordered_set<std::string> last_diagnostics_files_;
    // Implements parser_library::diagnostics_consumer: wraps the diagnostics in json and
    // sends them to client.
    virtual void consume_diagnostics(parser_library::diagnostic_list diagnostics) override;

    // Registers LSP methods implemented by this server (not by features).
    void register_methods();
};

} // namespace hlasm_plugin::language_server::lsp


#endif
