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

#ifndef HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_H
#define HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_H

#include <atomic>
#include <thread>

#include "../json_queue_channel.h"
#include "../message_router.h"
#include "dap_message_wrappers.h"
#include "telemetry_sink.h"
#include "workspace_manager.h"

namespace hlasm_plugin::language_server {
class external_file_reader;
}

namespace hlasm_plugin::language_server::dap {

class session final : public json_sink
{
    std::string session_id;
    hlasm_plugin::parser_library::workspace_manager* ws_mngr;
    json_queue_channel queue;
    dap::message_wrapper msg_wrapper;
    dap::message_unwrapper msg_unwrapper;
    std::thread worker;
    std::atomic<bool> running = true;
    telemetry_sink* telemetry_reporter;
    external_file_reader* ext_files;

    void thread_routine();

public:
    session(size_t session_id,
        hlasm_plugin::parser_library::workspace_manager& ws,
        json_sink& out,
        telemetry_sink* telemetry_reporter = nullptr,
        external_file_reader* ext_files = nullptr);
    ~session();

    message_router::message_predicate get_message_matcher() const;

    [[nodiscard]] bool is_running() const noexcept { return running; }

    [[nodiscard]] const std::string& get_session_id() const noexcept { return session_id; }

    // Inherited via json_sink
    void write(const nlohmann::json&) override;
    void write(nlohmann::json&&) override;
};
} // namespace hlasm_plugin::language_server::dap
#endif // HLASMPLUGIN_LANGUAGESERVER_DAP_SESSION_H
