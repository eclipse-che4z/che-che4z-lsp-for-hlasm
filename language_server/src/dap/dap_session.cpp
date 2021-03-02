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

#include "dap_session.h"

#include "json.hpp"

#include "../dispatcher.h"
#include "../request_manager.h"
#include "../scope_exit.h"
#include "dap_server.h"

namespace hlasm_plugin::language_server::dap {
void session::thread_routine()
{
    scope_exit indicate_end([this]() { running = false; });
    request_manager req_mgr(cancel);
    scope_exit end_request_manager([&req_mgr]() { req_mgr.end_worker(); });
    dap::server server(*ws_mngr);
    dispatcher dispatcher(json_channel_adapter(msg_unwrapper, msg_wrapper), server, req_mgr);
    dispatcher.run_server_loop();
}
session::session(size_t s_id, std::atomic<bool>& c, hlasm_plugin::parser_library::workspace_manager& ws, json_sink& out)
    : session_id(message_wrapper::generate_method_name(s_id))
    , cancel(&c)
    , ws_mngr(&ws)
    , msg_wrapper(out, s_id)
    , msg_unwrapper(queue)
{
    worker = std::thread([this]() { this->thread_routine(); });
}
session::~session()
{
    queue.terminate();
    if (worker.joinable())
        worker.join();
}
message_router::message_predicate session::get_message_matcher() const
{
    return [session_id = session_id](const nlohmann::json& msg) {
        auto method_ptr = msg.find("method");
        return method_ptr != msg.end() && method_ptr->get<std::string_view>() == session_id;
    };
}
void session::write(const nlohmann::json& msg) { queue.write(msg); }
void session::write(nlohmann::json&& msg) { queue.write(std::move(msg)); }
} // namespace hlasm_plugin::language_server::dap
