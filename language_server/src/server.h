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

#ifndef HLASMPLUGIN_LANGUAGESERVER_SERVER
#define HLASMPLUGIN_LANGUAGESERVER_SERVER

#include <chrono>
#include <unordered_set>

#include "feature.h"
#include "send_message_provider.h"
#include "telemetry_sink.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server {

// Abstract class that calls the correct serving method for registered LSP and DAP notifications and requests
// Can be implemented to fit LSP or DAP. Class implementing this class must also implement response
// methods from response_provider
class server : public response_provider
{
public:
    // Constructs the server with workspace_manager.
    // All the requests and notifications are passed to the workspace manager
    explicit server(parser_library::workspace_manager& ws_mngr, telemetry_sink* telemetry_provider = nullptr);

    // Tells the server that a massage was received. The server carries out the notification or request.
    virtual void message_received(const nlohmann::json& message) = 0;

    // Returns true, if LSP shutdown request has been received.
    bool is_shutdown_request_received() const;
    // Returns true, if LSP exit notification has been received.
    bool is_exit_notification_received() const;

    void set_send_message_provider(send_message_provider* provider);

protected:
    send_message_provider* send_message_ = nullptr;

    std::vector<std::unique_ptr<feature>> features_;

    std::map<std::string, method> methods_;
    std::unordered_map<unsigned long long, method> request_handlers_;

    bool shutdown_request_received_ = false;
    bool exit_notification_received_ = false;

    parser_library::workspace_manager& ws_mngr_;
    telemetry_sink* telemetry_provider_;

    void register_feature_methods();

    // Calls a method that is registered in methods_ with the specified name with arguments and id of request.
    void call_method(const std::string& method, const nlohmann::json& id, const nlohmann::json& args);

    virtual telemetry_metrics_info get_telemetry_details();

    void send_telemetry_error(std::string where, std::string what = "");

private:
    void telemetry_method_call(const std::string& method_name, telemetry_log_level log_level, double seconds);
};

} // namespace hlasm_plugin::language_server
#endif
