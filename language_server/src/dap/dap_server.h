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

#include "json.hpp"

#include "../common_types.h"
#include "../feature.h"
#include "../server.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::dap {

// Implements DAP server (session-controlling methods like initialize and disconnect).
// Integrates 1 feature: feature launch.
class server : public hlasm_plugin::language_server::server
{
public:
    explicit server(parser_library::workspace_manager& ws_mngr);

    // Inherited via server
    virtual void respond(const json& id, const std::string& requested_method, const json& args) override;

    virtual void notify(const std::string& method, const json& args) override;

    virtual void respond_error(const json& id,
        const std::string& requested_method,
        int err_code,
        const std::string& err_message,
        const json& error) override;

    virtual void message_received(const json& message) override;



private:
    uint64_t last_seq_ = 0;

    void on_initialize(const json& requested_seq, const json& args);
    void on_disconnect(const json& request_seq, const json& args);

    void register_methods();
};

} // namespace hlasm_plugin::language_server::dap


#endif
