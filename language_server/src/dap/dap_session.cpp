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
#include "external_file_reader.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "utils/scope_exit.h"

namespace hlasm_plugin::language_server::dap {
void session::thread_routine()
{
    try
    {
        const auto ext = [this]() {
            if (ext_files)
                return ext_files->register_thread([this]() noexcept {
                    // terminates on failure
                    queue.write(nlohmann::json::value_t::discarded);
                });
            else
                return external_file_reader::thread_registration();
        }();

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

        dap::server server(*this, telemetry_reporter);
        server.set_send_message_provider(&smp);

        while (!server.is_exit_notification_received())
        {
            if (queue.will_read_block())
                server.idle_handler(queue.will_block_preview());

            auto msg = channel.read();
            if (!msg.has_value())
                break;

            if (msg->is_discarded())
                continue;

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
void session::provide_debugger_configuration(parser_library::sequence<char> document_uri,
    parser_library::workspace_manager_response<parser_library::debugging::debugger_configuration> conf)
{
    struct proxy
    {
        parser_library::workspace_manager_response<parser_library::debugging::debugger_configuration> conf;
        json_queue_channel& queue;

        void wakeup() const noexcept
        {
            // terminates on failure
            queue.write(nlohmann::json::value_t::discarded);
        }

        void provide(parser_library::debugging::debugger_configuration&& r) const
        {
            conf.provide(std::move(r));
            wakeup();
        }
        void error(int err, const char* errmsg) const noexcept
        {
            conf.error(err, errmsg);
            wakeup();
        }
    };
    dc_provider->provide_debugger_configuration(
        document_uri, parser_library::make_workspace_manager_response(proxy { std::move(conf), queue }).first);
}
session::session(size_t s_id,
    parser_library::debugger_configuration_provider& dc_provider,
    json_sink& out,
    telemetry_sink* telem_reporter,
    external_file_reader* ext_files)
    : session_id(message_wrapper::generate_method_name(s_id))
    , dc_provider(&dc_provider)
    , msg_wrapper(out, s_id)
    , msg_unwrapper(queue)
    , telemetry_reporter(telem_reporter)
    , ext_files(ext_files)
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
