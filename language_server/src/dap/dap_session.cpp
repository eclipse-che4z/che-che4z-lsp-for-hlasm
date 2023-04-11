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

#include "dap_server.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/scope_exit.h"

namespace hlasm_plugin::language_server::dap {
void session::thread_routine()
{
    try
    {
        json_channel_adapter channel(msg_unwrapper, msg_wrapper);
        struct smp_t final : send_message_provider
        {
            json_channel_adapter& channel;
            void reply(const nlohmann::json& result) override { channel.write(result); }
            explicit smp_t(json_channel_adapter& channel)
                : channel(channel)
            {}
        } smp(channel);

        utils::scope_exit indicate_end([this]() noexcept { running = false; });

        dap::server server(*ws_mngr, telemetry_reporter);
        server.set_send_message_provider(&smp);

        while (!server.is_exit_notification_received())
        {
            if (queue.will_read_block() && server.idle_handler(queue.will_block_preview()))
                continue;

            auto msg = channel.read();
            if (!msg.has_value())
                break;
            server.message_received(msg.value());
        }
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(std::string("DAP Thread exception encountered: ") + ex.what());
    }
    catch (...)
    {
        LOG_ERROR("DAP Thread encountered an unknown exception.");
    }
}
session::session(
    size_t s_id, hlasm_plugin::parser_library::workspace_manager& ws, json_sink& out, telemetry_sink* telem_reporter)
    : session_id(message_wrapper::generate_method_name(s_id))
    , ws_mngr(&ws)
    , msg_wrapper(out, s_id)
    , msg_unwrapper(queue)
    , telemetry_reporter(telem_reporter)
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
