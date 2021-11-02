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

#include <chrono>
#include <thread>

#include "dap/dap_message_wrappers.h"
#include "dap/dap_server.h"
#include "dap/dap_session.h"
#include "dap/dap_session_manager.h"
#include "dispatcher.h"
#include "logger.h"
#include "lsp/lsp_server.h"
#include "message_router.h"
#include "scope_exit.h"
#include "server_streams.h"
#include "telemetry_broker.h"
#include "workspace_manager.h"

using namespace hlasm_plugin::language_server;

namespace {

class main_program : public json_sink
{
    std::atomic<bool> cancel = false;
    hlasm_plugin::parser_library::workspace_manager ws_mngr;

    json_queue_channel lsp_queue;

    message_router router;

    std::thread lsp_thread;
    telemetry_broker dap_telemetry_broker;
    dap::session_manager dap_sessions;

public:
    main_program(json_sink& json_output, int& ret)
        : ws_mngr(&cancel)
        , router(&lsp_queue)
        , dap_sessions(ws_mngr, json_output, &dap_telemetry_broker)
    {
        router.register_route(dap_sessions.get_filtering_predicate(), dap_sessions);

        lsp_thread = std::thread([&ret, this, io = json_channel_adapter(lsp_queue, json_output)]() {
            try
            {
                request_manager req_mgr(&cancel);
                scope_exit end_request_manager([&req_mgr]() { req_mgr.end_worker(); });
                lsp::server server(ws_mngr);
                scope_exit disconnect_telemetry([this]() { dap_telemetry_broker.set_telemetry_sink(nullptr); });
                dap_telemetry_broker.set_telemetry_sink(&server);

                dispatcher lsp_dispatcher(io, server, req_mgr);
                ret = lsp_dispatcher.run_server_loop();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR(std::string("LSP thread exception: ") + e.what());
            }
            catch (...)
            {
                LOG_ERROR("LSP thread unknown exception.");
            }
        });
    }
    ~main_program()
    {
        cancel = true;
        lsp_queue.terminate();
        if (lsp_thread.joinable())
            lsp_thread.join();
    }
    main_program(const main_program&) = delete;
    main_program(main_program&&) = delete;

    // Inherited via json_sink
    void write(const nlohmann::json& msg) override { router.write(msg); }
    void write(nlohmann::json&& msg) override { router.write(std::move(msg)); }
};

} // namespace

int main(int argc, char** argv)
{
    using namespace hlasm_plugin::language_server;

    auto io_setup = server_streams::create(argc, argv);
    if (!io_setup)
        return 1;

    try
    {
        int ret = 0;

        main_program pgm(io_setup->get_response_stream(), ret);

        for (auto& source = io_setup->get_request_stream();;)
        {
            auto msg = source.read();

            if (!msg.has_value())
                break;

            try
            {
                pgm.write(msg.value());
            }
            catch (const nlohmann::json::exception&)
            {
                LOG_WARNING("Could not parse received JSON: " + msg.value());
            }
        }

        return ret;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR(ex.what());
        return 1;
    }
    catch (...)
    {
        LOG_ERROR("Unknown error occured. Terminating.");
        return 2;
    }
}
