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
class session_manager final : public json_sink
{
    hlasm_plugin::parser_library::workspace_manager* ws_mngr;
    json_sink* out_stream;
    std::map<std::string, std::unique_ptr<dap::session>, std::less<>> sessions;
    json_sink* telemetry_reporter;

    void cleanup_sessions();
    void handle_registration_request(size_t new_id);

public:
    session_manager(hlasm_plugin::parser_library::workspace_manager& ws, json_sink& out, json_sink* telemetry_reporter = nullptr);

    // Inherited via json_sink
    void write(const nlohmann::json& msg) override;
    void write(nlohmann::json&& msg) override;

    [[nodiscard]] message_router::message_predicate get_filtering_predicate() const;

    [[nodiscard]] size_t registered_sessions_count() const { return sessions.size(); }
};
} // namespace hlasm_plugin::language_server::dap

#endif // HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_MANAGER_H
