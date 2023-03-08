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


#include "server.h"

#include <chrono>
#include <functional>
#include <sstream>
#include <string>

#include "logger.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server {

server::server(parser_library::workspace_manager& ws_mngr, telemetry_sink* telemetry_provider)
    : ws_mngr_(ws_mngr)
    , telemetry_provider_(telemetry_provider)
{}

void server::register_feature_methods()
{
    for (auto& f : features_)
    {
        f->register_methods(methods_);
    }
}

void server::call_method(const std::string& method, const nlohmann::json& id, const nlohmann::json& args)
{
    if (shutdown_request_received_)
    {
        LOG_WARNING("Request " + method + " was received after shutdown request.");
    }

    auto found = methods_.find(method);
    if (found != methods_.end())
    {
        try
        {
            auto start = std::chrono::steady_clock::now();
            (*found).second.handler(id.is_null() ? /* for compatibility */ nlohmann::json("") : id, args);
            std::chrono::duration<double> duration = std::chrono::steady_clock::now() - start;

            telemetry_method_call(method, (*found).second.telemetry_level, duration.count());
        }
        catch (const nlohmann::basic_json<>::exception& e)
        {
            (void)e;
            LOG_WARNING("There is an error regarding the JSON or LSP:" + std::string(e.what()));
            send_telemetry_error("call_method/json_error");
        }
    }
    else if (method.starts_with("$/"))
    {
        // LSP spec says:
        // - notification can be ignored
        // - requests should be responded to with MethodNotFound
        if (!id.is_null())
            send_message_->reply(nlohmann::json {
                { "id", id },
                {
                    "error",
                    {
                        { "code", -32601 },
                        { "message", "MethodNotFound" },
                    },
                },
            });
    }
    else
    {
        std::ostringstream ss;
        ss << "Method " << method << " is not available on this server.";
        LOG_WARNING(ss.str());

        send_telemetry_error("method_not_implemented", method);
    }
}

telemetry_metrics_info server::get_telemetry_details() { return {}; }

void server::send_telemetry_error(std::string where, std::string what)
{
    if (!telemetry_provider_)
        return;

    telemetry_provider_->send_telemetry(telemetry_error { where, what });
}

void server::telemetry_method_call(const std::string& method_name, telemetry_log_level log_level, double seconds)
{
    if (log_level == telemetry_log_level::NO_TELEMETRY)
        return;
    if (!telemetry_provider_)
        return;

    telemetry_info info { method_name, seconds };

    if (log_level == telemetry_log_level::LOG_WITH_PARSE_DATA)
        info.metrics = get_telemetry_details();

    telemetry_provider_->send_telemetry(info);
}

bool server::is_exit_notification_received() const { return exit_notification_received_; }

void server::set_send_message_provider(send_message_provider* provider) { send_message_ = provider; }

bool server::is_shutdown_request_received() const { return shutdown_request_received_; }

} // namespace hlasm_plugin::language_server
