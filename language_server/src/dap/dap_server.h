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

#include <functional>
#include <memory>
#include <unordered_set>

#include "../common_types.h"
#include "../server.h"
#include "dap_feature.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server::dap {

// Implements DAP server (session-controlling methods like initialize and disconnect).
// Integrates 1 feature: feature launch.
class server final : public hlasm_plugin::language_server::server, public dap_disconnect_listener
{
public:
    explicit server(parser_library::workspace_manager& ws_mngr, telemetry_sink* telemetry_reporter = nullptr);

    void request(const json& id, const std::string& requested_method, const json& args, method handler) override;

    void respond(const json& id, const std::string& requested_method, const json& args) override;

    void notify(const std::string& method, const json& args) override;

    void respond_error(const json& id,
        const std::string& requested_method,
        int err_code,
        const std::string& err_message,
        const json& error) override;

    void message_received(const json& message) override;

private:
    std::atomic<uint64_t> last_seq_ = 0;

    void register_methods();

    // Inherited via dap_disconnect_listener
    void disconnected() override;
};

} // namespace hlasm_plugin::language_server::dap


#endif
