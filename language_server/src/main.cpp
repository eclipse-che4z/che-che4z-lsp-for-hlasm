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

#include <optional>
#include <span>
#include <thread>

#include "encodings.h"
#include "server_options.h"
#include "utils/content_loader.h"
#ifdef WIN32
#    include <locale.h>
#endif

#include "json_queue_channel.h"

#include "dap/dap_session_manager.h"
#include "external_file_reader.h"
#include "logger.h"
#include "lsp/lsp_server.h"
#include "message_router.h"
#include "nlohmann/json.hpp"
#include "server_streams.h"
#include "telemetry_broker.h"
#include "utils/scope_exit.h"
#include "virtual_file_provider.h"
#include "workspace_manager.h"

using namespace hlasm_plugin::language_server;

namespace {

class main_program final : public json_sink,
                           send_message_provider,
                           hlasm_plugin::parser_library::debugger_configuration_provider
{
    external_file_reader external_files;
    std::unique_ptr<hlasm_plugin::parser_library::workspace_manager> ws_mngr;

    hlasm_plugin::parser_library::debugger_configuration_provider& dc_provider;
    std::mutex proxies_mutex;
    std::deque<std::function<void()>> proxies;

    json_sink& json_output;
    json_queue_channel lsp_queue;

    message_router router;

    std::thread lsp_thread;
    telemetry_broker dap_telemetry_broker;
    dap::session_manager dap_sessions;
    virtual_file_provider virtual_files;

    void reply(const nlohmann::json& message) final { json_output.write(message); }

    void provide_debugger_configuration(std::string_view document_uri,
        hlasm_plugin::parser_library::workspace_manager_response<
            hlasm_plugin::parser_library::debugging::debugger_configuration> conf) override
    {
        std::unique_lock g(proxies_mutex);
        proxies.emplace_back([this, uri = std::string(document_uri), conf = std::move(conf)]() {
            dc_provider.provide_debugger_configuration(uri, conf);
        });
        g.unlock();
        lsp_queue.write(nlohmann::json::value_t::discarded);
    }

public:
    main_program(json_sink& json_output, int& ret, bool use_vscode_extensions)
        : external_files(json_output)
        , ws_mngr(hlasm_plugin::parser_library::create_workspace_manager(&external_files, use_vscode_extensions))
        , dc_provider(ws_mngr->get_debugger_configuration_provider())
        , json_output(json_output)
        , router(&lsp_queue)
        , dap_sessions(*this, json_output, &dap_telemetry_broker)
        , virtual_files(*ws_mngr, json_output)
    {
        router.register_route(dap_sessions.get_filtering_predicate(), dap_sessions);
        router.register_route(virtual_files.get_filtering_predicate(), virtual_files);
        router.register_route(external_files.get_filtering_predicate(), external_files);

        lsp_thread = std::thread([&ret, this]() {
            try
            {
                auto ext_reg = external_files.register_thread([this]() noexcept {
                    // terminates on failure
                    lsp_queue.write(nlohmann::json::value_t::discarded);
                });

                lsp::server server(*ws_mngr);
                server.set_send_message_provider(this);

                hlasm_plugin::utils::scope_exit disconnect_telemetry(
                    [this]() noexcept { dap_telemetry_broker.set_telemetry_sink(nullptr); });
                dap_telemetry_broker.set_telemetry_sink(&server);

                for (;;)
                {
                    if (lsp_queue.will_read_block())
                        ws_mngr->idle_handler(lsp_queue.will_block_preview());

                    auto message = lsp_queue.read();
                    if (!message.has_value())
                    {
                        ret = 1;
                        break;
                    }

                    if (message->is_discarded())
                    {
                        for (std::unique_lock g(proxies_mutex); !proxies.empty();)
                        {
                            proxies.front()();
                            proxies.pop_front();
                        }
                        continue;
                    }

                    server.message_received(message.value());

                    // If exit notification came without prior shutdown request, return error 1.
                    if (server.is_exit_notification_received())
                    {
                        if (!server.is_shutdown_request_received())
                            ret = 1;
                        break;
                    }
                }
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("LSP thread exception: ", e.what());
                ret = -1;
            }
            catch (...)
            {
                LOG_ERROR("LSP thread unknown exception.");
                ret = -1;
            }
        });
    }
    ~main_program()
    {
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

auto separate_arguments(int argc, char** argv)
{
    auto first = std::find_if(argv + !!argc, argv + argc, [](std::string_view arg) { return arg == "--hlasm-start"; });
    auto last = std::find_if(first, argv + argc, [](std::string_view arg) { return arg == "--hlasm-end"; });

    if (last == argv + argc)
        first = argv + !!argc;
    else
        ++first;

    return std::span<const char* const>(first, last - first);
}

void log_options(server_options opts)
{
    LOG_INFO("Options: vscode-extensions=",
        opts.enable_vscode_extension ? "true" : "false",
        ", log-level=",
        std::to_string(opts.log_level),
        ", lsp-port=",
        std::to_string(opts.port),
        ", translate-text=",
        encodings_to_text(opts.translate_text));
}

} // namespace

int main(int argc, char** argv)
{
#ifdef WIN32
    setlocale(LC_ALL, ".UTF-8");
#endif
    using namespace hlasm_plugin::language_server;

    auto args = separate_arguments(argc, argv);

    auto opts = parse_options(args);
    if (!opts)
        return 1;

    if (opts->log_level >= 0)
        logger::instance.level(opts->log_level);

    log_options(*opts);

    if (const auto* e = get_input_encoding(opts->translate_text); e)
        hlasm_plugin::utils::resource::set_content_loader_translation(e);

    auto io_setup = server_streams::create(*opts);
    if (!io_setup)
        return 1;

    try
    {
        int ret = 0;

        main_program pgm(io_setup->get_response_stream(), ret, opts->enable_vscode_extension);

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
                LOG_WARNING("Could not parse received JSON: ",
                    msg.value().dump(-1, ' ', true, nlohmann::detail::error_handler_t::replace));
            }
        }

        return ret;
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Main loop exception: ", ex.what());
        return 1;
    }
    catch (...)
    {
        LOG_ERROR("Unknown error occured. Terminating.");
        return 2;
    }
}
