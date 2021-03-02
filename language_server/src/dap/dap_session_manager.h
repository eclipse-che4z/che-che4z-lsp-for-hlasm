/*
 * Copyright (c) 2021 Broadcom.
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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_MANAGER_H
#define HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_MANAGER_H
#include <atomic>
#include <map>
#include <memory>
#include <optional>

#include "../json_channel.h"
#include "../message_router.h"
#include "dap_session.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server::dap {
class session_manager : public json_sink
{
    std::atomic<bool>* cancel;
    hlasm_plugin::parser_library::workspace_manager* ws_mngr;
    json_sink* out_stream;
    std::map<std::string, std::unique_ptr<dap::session>, std::less<>> sessions;

    void cleanup_sessions();
    void handle_registration_request(size_t new_id);

public:
    session_manager(std::atomic<bool>& c, hlasm_plugin::parser_library::workspace_manager& ws, json_sink& out);

    // Inherited via json_sink
    void write(const nlohmann::json& msg) override;
    void write(nlohmann::json&& msg) override;

    message_router::message_predicate get_filtering_predicate() const;
};
} // namespace hlasm_plugin::language_server::dap

#endif // HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_MANAGER_H
