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

#include "stream_helper.h"

#ifndef __EMSCRIPTEN__
#    define ASIO_STANDALONE
#    include "asio.hpp"
#    include "asio/stream_socket_service.hpp"
#    include "asio/system_error.hpp"

#    include "base_protocol_channel.h"
#else
#    include <emscripten.h>
#    include <string>

#    include <emscripten/bind.h>
#    include <emscripten/val.h>

#    include "blocking_queue.h"
#    include "logger.h"
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

#ifdef __EMSCRIPTEN__

class emscripten_std_setup : public server_streams, public json_source, public json_sink
{
    blocking_queue<std::string, blocking_queue_termination_policy::process_elements> queue;

    std::string stdin_buffer;

    intptr_t get_ptr_token() const { return reinterpret_cast<intptr_t>(this); }

public:
    static emscripten::val get_stdin_buffer(intptr_t ptr_val, ssize_t len)
    {
        auto* ptr = reinterpret_cast<emscripten_std_setup*>(ptr_val);
        ptr->stdin_buffer.resize(len);
        return emscripten::val(emscripten::typed_memory_view(len, (unsigned char*)ptr->stdin_buffer.data()));
    }

    static void commit_stdin_buffer(intptr_t ptr_val)
    {
        auto* ptr = reinterpret_cast<emscripten_std_setup*>(ptr_val);

        ptr->queue.push(std::move(ptr->stdin_buffer));
    }

    static void terminate_input(intptr_t ptr_val)
    {
        auto* ptr = reinterpret_cast<emscripten_std_setup*>(ptr_val);
        ptr->queue.terminate();
    }

    json_sink& get_response_stream() & override { return *this; }

    json_source& get_request_stream() & override { return *this; }

    void write(const nlohmann::json& msg) override
    {
        auto msg_string = msg.dump();
        auto msg_string_size = msg_string.size();
        auto msg_to_send = std::string("Content-Length: ") + std::to_string(msg_string_size) + std::string("\r\n\r\n")
            + std::move(msg_string);


        MAIN_THREAD_EM_ASM(
            { process.stdout.write(HEAPU8.slice($0, $0 + $1)); }, msg_to_send.data(), msg_to_send.size());
    }
    void write(nlohmann::json&& msg) override { write(msg); }

    std::optional<nlohmann::json> read() override
    {
        while (true)
        {
            auto msg = queue.pop();
            if (!msg.has_value())
                return std::nullopt;

            try
            {
                return nlohmann::json::parse(msg.value());
            }
            catch (const nlohmann::json::exception&)
            {
                LOG_WARNING("Could not parse received JSON: " + msg.value());
            }
        }
    }

    emscripten_std_setup()
    {
        MAIN_THREAD_EM_ASM(
            {
                const content_length = 'Content-Length: ';
                var buffer = Buffer.from([]);

                const ptr = $0;
                Module["emscripten_std_setup_term"] = Module["emscripten_std_setup_term"] || new Map();

                function close_event_handler() { Module.terminate_input(ptr); };
                function data_event_handler(data)
                {
                    buffer = Buffer.concat([ buffer, data ]);
                    while (true)
                    {
                        if (buffer.indexOf(content_length) != 0)
                            return;
                        const end_of_line = buffer.indexOf('\\x0D\\x0A');
                        if (end_of_line < 0)
                            return;
                        const length = +buffer.slice(content_length.length, end_of_line);
                        const end_of_headers = buffer.indexOf('\\x0D\\x0A\\x0D\\x0A');
                        if (end_of_headers < 0)
                            return;
                        const data_start = end_of_headers + 4;
                        const data_end = data_start + length;
                        if (data_end > buffer.length)
                            return;

                        const data_to_pass = buffer.slice(data_start, data_end);
                        buffer = buffer.slice(data_end);

                        const store_buffer = Module.get_stdin_buffer(ptr, data_to_pass.length);
                        data_to_pass.copy(store_buffer);
                        Module.commit_stdin_buffer(ptr);
                    }
                };
                process.stdin.on('close', close_event_handler);
                process.stdin.on('data', data_event_handler);

                Module["emscripten_std_setup_term"][ptr] = function()
                {
                    process.stdin.removeListener('close', close_event_handler);
                    process.stdin.removeListener('data', data_event_handler);
                };
            },
            get_ptr_token());
    }

    ~emscripten_std_setup()
    {
        MAIN_THREAD_EM_ASM(
            {
                const ptr = $0;
                Module["emscripten_std_setup_term"][ptr]();
                delete Module["emscripten_std_setup_term"][ptr];
            },
            get_ptr_token());
    }
};

EMSCRIPTEN_BINDINGS(main_thread)
{
    emscripten::function("get_stdin_buffer", &emscripten_std_setup::get_stdin_buffer);
    emscripten::function("commit_stdin_buffer", &emscripten_std_setup::commit_stdin_buffer);
    emscripten::function("terminate_input", &emscripten_std_setup::terminate_input);
}

#else

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

    return std::make_unique<emscripten_std_setup>();
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
        return std::make_unique<tcp_setup>((uint16_t)lsp_port);
    }
    else
        return std::make_unique<std_setup>();
}

#endif // __EMSCRIPTEN__

} // namespace hlasm_plugin::language_server