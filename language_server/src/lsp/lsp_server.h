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

#include "../server.h"
#include "../telemetry_sink.h"
#include "nlohmann/json_fwd.hpp"
#include "progress_notification.h"
#include "watcher_registration_provider.h"
#include "workspace_manager.h"
#include "workspace_manager_requests.h"

namespace hlasm_plugin::language_server::lsp {

// Implements LSP server (session-controlling methods like initialize and shutdown).
// Integrates 3 features: language features, text synchronization and workspace folders.
// Consumes diagnostics that come from the parser library and sends them to LSP client.
class server final : public hlasm_plugin::language_server::server,
                     parser_library::diagnostics_consumer,
                     parser_library::message_consumer,
                     public telemetry_sink,
                     parser_library::parsing_metadata_consumer,
                     parser_library::workspace_manager_requests,
                     parser_library::watcher_registration_provider
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
    void request(std::string_view requested_method,
        nlohmann::json&& args,
        std::function<void(const nlohmann::json& params)> handler,
        std::function<void(int, const char*)> error_handler) override;
    // Sends respond to request to LSP client using send_message_provider.
    void respond(const request_id& id, std::string_view requested_method, nlohmann::json&& args) override;
    // Sends notification to LSP client using send_message_provider.
    void notify(std::string_view method, nlohmann::json&& args) override;
    // Sends erroneous respond to LSP client using send_message_provider.
    void respond_error(const request_id& id,
        std::string_view requested_method,
        int err_code,
        std::string_view err_message,
        nlohmann::json&& error) override;

    void register_cancellable_request(const request_id&, request_invalidator) override;

private:
    std::atomic<long> request_id_counter = 0;
    parser_library::workspace_manager& ws_mngr;
    progress_notification progress;

    struct watcher_registration
    {
        std::string base_uri;
        bool recursive;
        parser_library::watcher_registration_id id;
        unsigned long long reference_count = 1;
    };

    parser_library::watcher_registration_id m_next_watcher_id = parser_library::watcher_registration_id::INVALID;
    std::vector<watcher_registration> m_watcher_registrations;

    bool m_supports_dynamic_file_change_notification : 1 = false;
    bool m_supports_file_change_notification_relative_pattern : 1 = false;

    // requests
    // Implements initialize request.
    void on_initialize(const request_id& id, const nlohmann::json& param);
    // Implements initialized notification.
    void on_initialized(const nlohmann::json&);
    // Implements the LSP shutdown request.
    void on_shutdown(const request_id& id, const nlohmann::json& param);

    // notifications

    // Implements the LSP exit request.
    void on_exit(const nlohmann::json& param);


    // client notifications

    // Implements the LSP showMessage request.
    void show_message(std::string_view message, parser_library::message_type type) override;

    // Remembers name of files for which were sent diagnostics the last time
    // diagnostics were sent to client. Used to clear diagnostics in client
    // when no more diags are produced by server for particular file.
    std::unordered_set<std::string> last_diagnostics_files_;
    // Implements parser_library::diagnostics_consumer: wraps the diagnostics in json and
    // sends them to client.
    void consume_diagnostics(std::span<const parser_library::diagnostic> diagnostics,
        std::span<const parser_library::fade_message> fade_messages) override;

    // Registers LSP methods implemented by this server (not by features).
    void register_methods();

    // Ingest parsing metadata and forward them to telemetry client
    void consume_parsing_metadata(
        std::string_view uri, double duration, const parser_library::parsing_metadata& metadata) override;

    // Notify vscode that outputs have changed
    void outputs_changed(std::string_view uri) override;

    void request_workspace_configuration(
        std::string_view url, parser_library::workspace_manager_response<std::string_view> json_text) override;
    void request_file_configuration(
        std::string_view url, parser_library::workspace_manager_response<std::string_view> json_text) override;

    void invalidate_external_configuration(const nlohmann::json& error);

    void toggle_advisory_configuration_diagnostics(const nlohmann::json&);

    void set_log_level(const nlohmann::json&);

    parser_library::watcher_registration_id add_watcher(std::string_view uri, bool recursive) override;
    void remove_watcher(parser_library::watcher_registration_id id) override;

    parser_library::watcher_registration_id next_watcher_id() noexcept;
    void register_default_watcher();
    void fill_change_notification_support_flags(const nlohmann::json& json);

public: // testing only
    void testing_enable_capabilities();
};

} // namespace hlasm_plugin::language_server::lsp


#endif
