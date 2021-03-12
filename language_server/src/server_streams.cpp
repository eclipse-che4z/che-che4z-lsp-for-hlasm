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

#include "server_streams.h"

#include <mutex>
#include <stdexcept>

#include "base_protocol_channel.h"
#include "stream_helper.h"

#ifndef __EMSCRIPTEN__
#    define ASIO_STANDALONE
#    include "asio.hpp"
#    include "asio/stream_socket_service.hpp"
#    include "asio/system_error.hpp"
#else
#    include <emscripten.h>
#    include <streambuf>

#    include <emscripten/bind.h>
#    include <emscripten/val.h>
#endif // !__EMSCRIPTEN__

#ifdef _WIN32 // set binary mode for input on windows
#    include <fcntl.h>
#    include <io.h>
#    define SET_BINARY_MODE(handle) _setmode(_fileno(handle), O_BINARY)
#else
#    define SET_BINARY_MODE(handle)
#endif
// no need for binary on linux, because it does not change \n into \r\n

namespace hlasm_plugin::language_server {
namespace {

class std_setup final : public server_streams
{
    base_protocol_channel channel;

public:
    std_setup()
        : channel(std::cin, std::cout)
    {
        SET_BINARY_MODE(stdin);
        SET_BINARY_MODE(stdout);
        newline_is_space::imbue_stream(std::cin);
    }

    json_sink& get_response_stream() & override { return channel; }

    void feed_requests(json_sink& pgm) override
    {
        for (;;)
        {
            auto msg = channel.read();
            if (!msg.has_value())
                break;
            pgm.write(std::move(msg).value());
        }
    }
};

#ifndef __EMSCRIPTEN__
class tcp_setup final : public server_streams
{
    asio::io_service io_service;
    asio::ip::tcp::acceptor acceptor;
    asio::ip::tcp::socket socket;
    asio::ip::tcp::iostream stream;

    base_protocol_channel channel;

public:
    explicit tcp_setup(uint16_t port)
        : acceptor(io_service, asio::ip::tcp::endpoint(asio::ip::address::from_string("127.0.0.1"), port))
        , socket(io_service)
        , channel(stream, stream)
    {
        acceptor.accept(stream.socket());

        newline_is_space::imbue_stream(stream);
    }

    ~tcp_setup() { stream.close(); }

    json_sink& get_response_stream() & override { return channel; }

    void feed_requests(json_sink& pgm) override
    {
        for (;;)
        {
            auto msg = channel.read();
            if (!msg.has_value())
                break;
            pgm.write(std::move(msg).value());
        }
    }
};

#endif // !__EMSCRIPTEN__
} // namespace


#ifdef __EMSCRIPTEN__

std::unique_ptr<server_streams> server_streams::create(int argc, char** argv)
{
    (void)argv;
    if (argc != 1)
    {
        std::cerr << "No arguments allowed";
        return {};
    }

    return std::make_unique<std_setup>();
}

#else

std::unique_ptr<server_streams> server_streams::create(int argc, char** argv)
{
    if (argc > 2)
    {
        std::cerr << "Invalid arguments. Use language_server [<lsp port>]";
        return {};
    }
    const bool use_tcp = argc == 2;
    int lsp_port = 0;
    if (use_tcp)
    {
        lsp_port = atoi(argv[1]);
        if (lsp_port <= 0 || lsp_port > 65535)
        {
            std::cerr << "Wrong port entered.";
            return {};
        }
        return std::make_unique<tcp_setup>(lsp_port);
    }
    else
        return std::make_unique<std_setup>();
}

#endif // __EMSCRIPTEN__

} // namespace hlasm_plugin::language_server