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
#include <iostream>

#include "base_protocol_channel.h"
#include "server_streams.h"
#include "stream_helper.h"

#define ASIO_STANDALONE
#include "asio.hpp"
#include "asio/system_error.hpp"

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

class stdio_setup final : public server_streams
{
    base_protocol_channel channel;

public:
    stdio_setup()
        : channel(std::cin, std::cout)
    {
        SET_BINARY_MODE(stdin);
        SET_BINARY_MODE(stdout);
        newline_is_space::imbue_stream(std::cin);
    }

    json_sink& get_response_stream() & override { return channel; }

    json_source& get_request_stream() & override { return channel; }
};

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

    json_source& get_request_stream() & override { return channel; }
};

} // namespace

std::unique_ptr<server_streams> server_streams::create(int argc, char** argv)
{
    if (argc > 1)
    {
        std::cerr << "Invalid arguments. Use language_server [<lsp port>]";
        return {};
    }
    const bool use_tcp = argc == 1;
    int lsp_port = 0;
    if (use_tcp)
    {
        lsp_port = atoi(argv[0]);
        if (lsp_port <= 0 || lsp_port > 65535)
        {
            std::cerr << "Wrong port entered.";
            return {};
        }
        return std::make_unique<tcp_setup>((uint16_t)lsp_port);
    }
    else
        return std::make_unique<stdio_setup>();
}

} // namespace hlasm_plugin::language_server
