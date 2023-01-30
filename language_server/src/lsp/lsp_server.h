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

#include <atomic>
#include <string>
#include <unordered_set>

#include "../parsing_metadata_collector.h"
#include "../server.h"
#include "nlohmann/json_fwd.hpp"
#include "telemetry_sink.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::lsp {

// Implements LSP server (session-controlling methods like initialize and shutdown).
// Integrates 3 features: language features, text synchronization and workspace folders.
// Consumes diagnostics that come from the parser library and sends them to LSP client.
class server final : public hlasm_plugin::language_server::server,
                     public parser_library::diagnostics_consumer,
                     public parser_library::message_consumer,
                     public telemetry_sink
{
public:
    // Creates the server with workspace_manager as entry point to parser library.
    explicit server(parser_library::workspace_manager& ws_mngr);

    // Parses LSP (JSON RPC) message and calls corresponding method.
    void message_received(const nlohmann::json& message) override;

    // Inherited via telemetry_sink
    void send_telemetry(const telemetry_message& message) override;

protected:
    // Sends request to LSP client using send_message_provider.
    void request(const std::string& requested_method, const nlohmann::json& args, method handler) override;
    // Sends respond to request to LSP client using send_message_provider.
    void respond(const nlohmann::json& id, const std::string& requested_method, const nlohmann::json& args) override;
    // Sends notification to LSP client using send_message_provider.
    void notify(const std::string& method, const nlohmann::json& args) override;
    // Sends erroneous respond to LSP client using send_message_provider.
    void respond_error(const nlohmann::json& id,
        const std::string& requested_method,
        int err_code,
        const std::string& err_message,
        const nlohmann::json& error) override;

    telemetry_metrics_info get_telemetry_details() override;

private:
    std::atomic<unsigned long long> request_id_counter = 0;
    parsing_metadata_collector parsing_metadata_;
    size_t diags_warning_count = 0;
    size_t diags_error_count = 0;

    // requests
    // Implements initialize request.
    void on_initialize(nlohmann::json id, const nlohmann::json& param);
    // Implements the LSP shutdown request.
    void on_shutdown(nlohmann::json id, const nlohmann::json& param);


    // notifications

    // Implements the LSP exit request.
    void on_exit(nlohmann::json id, const nlohmann::json& param);


    // client notifications

    // Implements the LSP showMessage request.
    void show_message(const std::string& message, parser_library::message_type type) override;

    // Remembers name of files for which were sent diagnostics the last time
    // diagnostics were sent to client. Used to clear diagnostics in client
    // when no more diags are produced by server for particular file.
    std::unordered_set<std::string> last_diagnostics_files_;
    // Implements parser_library::diagnostics_consumer: wraps the diagnostics in json and
    // sends them to client.
    void consume_diagnostics(
        parser_library::diagnostic_list diagnostics, parser_library::fade_message_list fade_messages) override;

    // Registers LSP methods implemented by this server (not by features).
    void register_methods();
};

} // namespace hlasm_plugin::language_server::lsp


#endif
