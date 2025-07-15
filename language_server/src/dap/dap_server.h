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

#ifndef HLASMPLUGIN_HLASMLANGUAGESERVER_DAP_SERVER_H
#define HLASMPLUGIN_HLASMLANGUAGESERVER_DAP_SERVER_H

#include <atomic>

#include "../server.h"
#include "dap_feature.h"
#include "nlohmann/json_fwd.hpp"

namespace hlasm_plugin::language_server::dap {

// Implements DAP server (session-controlling methods like initialize and disconnect).
// Integrates 1 feature: feature launch.
class server final : public hlasm_plugin::language_server::server, public dap_disconnect_listener
{
public:
    explicit server(
        parser_library::debugger_configuration_provider& dc_provider, telemetry_sink* telemetry_reporter = nullptr);

    void respond(const request_id& id, std::string_view requested_method, nlohmann::json&& args) override;
    void respond_error(const request_id& id,
        std::string_view requested_method,
        int err_code,
        std::string_view err_message,
        nlohmann::json&& error) override;
    void request(std::string_view requested_method,
        nlohmann::json&& args,
        std::function<void(const nlohmann::json& params)> handler,
        std::function<void(int, const char*)> error_handler) override;
    void notify(std::string_view method, nlohmann::json&& args) override;

    void register_cancellable_request(const request_id& id, request_invalidator cancel_handler) override;

    void message_received(const nlohmann::json& message) override;

    void idle_handler(const std::atomic<unsigned char>* yield_indicator);

private:
    std::atomic<uint64_t> last_seq_ = 0;
    dap_feature* m_dap_feature = nullptr;

    void register_methods();

    // Inherited via dap_disconnect_listener
    void disconnected() override;
};

} // namespace hlasm_plugin::language_server::dap


#endif
