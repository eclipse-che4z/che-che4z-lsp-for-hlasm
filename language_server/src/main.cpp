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
#include <variant>

#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/stream_socket_service.hpp"
#include "asio/system_error.hpp"
#include "json_queue_channel.h"

#include "dap/dap_message_wrappers.h"
#include "dap/dap_server.h"
#include "dap/dap_session.h"
#include "dap/dap_session_manager.h"
#include "dispatcher.h"
#include "logger.h"
#include "lsp/channel.h"
#include "lsp/lsp_server.h"
#include "message_router.h"
#include "scope_exit.h"
#include "stream_helper.h"
#include "workspace_manager.h"

#ifdef _WIN32 // set binary mode for input on windows
#    include <fcntl.h>
#    include <io.h>
#    define SET_BINARY_MODE(handle) _setmode(_fileno(handle), O_BINARY)
#else
#    define SET_BINARY_MODE(handle)
#endif
// no need for binary on linux, because it does not change \n into \r\n

using namespace hlasm_plugin::language_server;

namespace {
struct tcp_setup
{
    asio::io_service io_service;
    asio::ip::tcp::acceptor acceptor;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::iostream stream;
    tcp_setup(uint16_t port)
        : acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port))
        , socket(io_service)
    {
        acceptor.accept(stream.socket());

        newline_is_space::imbue_stream(stream);
    }
    std::pair<std::istream&, std::ostream&> get_streams()
    {
        return std::pair<std::istream&, std::ostream&>(stream, stream);
    }
    ~tcp_setup() { stream.close(); }
};
struct std_setup
{
    std::pair<std::istream&, std::ostream&> get_streams()
    {
        return std::pair<std::istream&, std::ostream&>(std::cin, std::cout);
    }
};
} // namespace

int main(int argc, char** argv)
{
    using namespace std;
    using namespace hlasm_plugin::language_server;

    if (argc > 2)
    {
        std::cout << "Invalid arguments. Use language_server [<lsp port>]";
        return 1;
    }

    const bool use_tcp = argc == 2;
    int lsp_port = 0;
    if (use_tcp)
    {
        lsp_port = atoi(argv[1]);
        if (lsp_port <= 0 || lsp_port > 65535)
        {
            std::cout << "Wrong port entered.";
            return 1;
        }
    }

    std::atomic<bool> cancel = false;
    try
    {
        SET_BINARY_MODE(stdin);
        SET_BINARY_MODE(stdout);
        newline_is_space::imbue_stream(cin);

        hlasm_plugin::parser_library::workspace_manager ws_mngr(&cancel);

        json_queue_channel lsp_queue;

        message_router router(&lsp_queue);

        int ret;
        {
            std::variant<std_setup, tcp_setup> io_setup;
            if (use_tcp)
                io_setup.emplace<tcp_setup>((uint16_t)lsp_port);

            auto [in_stream, out_stream] = std::visit([](auto&& p) { return p.get_streams(); }, io_setup);
            lsp::channel channel(in_stream, out_stream);

            std::thread lsp_thread;
            scope_exit clean_up_threads([&]() {
                lsp_queue.terminate();
                if (lsp_thread.joinable())
                    lsp_thread.join();
            });

            dap::session_manager dap_sessions(cancel, ws_mngr, channel);
            router.register_route(dap_sessions.get_filtering_predicate(), dap_sessions);

            lsp_thread = std::thread([&ret, &ws_mngr, &cancel, io = json_channel_adapter(lsp_queue, channel)]() {
                request_manager req_mgr(&cancel);
                scope_exit end_request_manager([&req_mgr]() { req_mgr.end_worker(); });
                lsp::server server(ws_mngr);

                dispatcher lsp_dispatcher(io, server, req_mgr);
                ret = lsp_dispatcher.run_server_loop();
            });

            for (;;)
            {
                auto msg = channel.read();
                if (!msg.has_value())
                    break;
                router.write(std::move(msg).value());
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
