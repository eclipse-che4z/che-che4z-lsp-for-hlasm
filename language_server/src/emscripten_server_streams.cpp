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
#include <emscripten.h>
#include <iostream>
#include <string>

#include <emscripten/bind.h>

#include "blocking_queue.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include "server_streams.h"
#include "utils/platform.h"

namespace hlasm_plugin::language_server {
namespace {

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
        if (utils::platform::is_web())
        {
            MAIN_THREAD_EM_ASM(
                { Module['worker'].postMessage(JSON.parse(new TextDecoder().decode(HEAPU8.slice($0, $0 + $1)))); },
                msg_string.data(),
                msg_string.size());
        }
        else
        {
            msg_string = "Content-Length: " + std::to_string(msg_string_size) + "\r\n\r\n" + std::move(msg_string);
            MAIN_THREAD_EM_ASM(
                { process.stdout.write(HEAPU8.slice($0, $0 + $1)); }, msg_string.data(), msg_string.size());
        }
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
        if (utils::platform::is_web())
            MAIN_THREAD_EM_ASM(
                {
                    const ptr = $0;
                    Module["emscripten_std_setup_term"] = Module["emscripten_std_setup_term"] || new Map();
                    function consume(e)
                    {
                        const data_to_pass = new TextEncoder().encode(JSON.stringify(e.data));
                        const store_buffer = Module.get_stdin_buffer(ptr, data_to_pass.length);
                        store_buffer.set(data_to_pass, 0);
                        Module.commit_stdin_buffer(ptr);
                    }
                    for (const m of Module["tmpQueue"])
                        consume(m);
                    Module["tmpQueue"] = [];
                    Module["worker"].onmessage = consume;
                    Module["emscripten_std_setup_term"][ptr] = function() { Module["worker"].onmessage = undefined; };
                },
                get_ptr_token());
        else
            MAIN_THREAD_EM_ASM(
                {
                    const content_length = 'Content-Length: ';
                    var buffer = Buffer.from([]);

                    const ptr = $0;
                    Module["emscripten_std_setup_term"] = Module["emscripten_std_setup_term"] || new Map();

                    function end_event_handler() { Module.terminate_input(ptr); };
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
                    process.stdin.on('data', data_event_handler);
                    process.stdin.on('end', end_event_handler);

                    Module["emscripten_std_setup_term"][ptr] = function()
                    {
                        process.stdin.removeListener('data', data_event_handler);
                        process.stdin.removeListener('end', end_event_handler);
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

} // namespace

std::unique_ptr<server_streams> server_streams::create(std::span<const char* const> args)
{
    if (!args.empty())
    {
        std::cerr << "No arguments allowed";
        return {};
    }

    return std::make_unique<emscripten_std_setup>();
}

} // namespace hlasm_plugin::language_server
