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
#include "server_streams.h"

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

        MAIN_THREAD_EM_ASM(
            {
                const response = HEAPU8.slice($0, $0 + $1);
                const response_str = new TextDecoder().decode(response);
                console.log(response_str);
                const json = JSON.parse(response_str);
                Module["lspWriter"].write(json);
            },
            msg_string.data(),
            msg_string.size());
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
                const ptr = $0;
                function consumeMessage(json)
                {
                    console.log(json);
                    const data_to_pass = new TextEncoder().encode(JSON.stringify(json));
                    const store_buffer = Module.get_stdin_buffer(ptr, data_to_pass.length);
                    let store_uint = new Uint8Array(store_buffer);
                    for (let i = 0; i < data_to_pass.length; ++i)
                        store_buffer[i] = data_to_pass[i];
                    Module.commit_stdin_buffer(ptr);
                }
                for (const x of Module["lspQueue"])
                {
                    console.log("consuming");
                    consumeMessage(x);
                }
                console.log("Streams");
                Module["lspReader"].listen(consumeMessage);
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

std::unique_ptr<server_streams> server_streams::create(int argc, char** argv)
{
    (void)argv;
    MAIN_THREAD_EM_ASM({ console.log("LANGUAGE BACKEND IS RUNNING"); });
    if (argc != 0)
    {
        std::cerr << "No arguments allowed";
        return {};
    }

    return std::make_unique<emscripten_std_setup>();
}

} // namespace hlasm_plugin::language_server
