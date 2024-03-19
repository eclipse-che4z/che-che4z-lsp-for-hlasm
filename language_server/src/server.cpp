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

#include <functional>
#include <sstream>

#include "logger.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server {

server::server(telemetry_sink* telemetry_provider)
    : telemetry_provider_(telemetry_provider)
{}

void server::register_feature_methods()
{
    for (auto& f : features_)
    {
        f->register_methods(methods_);
    }
}

void server::call_method(const std::string& method, std::optional<request_id> id, const nlohmann::json& args)
{
    if (shutdown_request_received_)
    {
        LOG_WARNING("Request " + method + " was received after shutdown request.");
    }

    auto found = methods_.find(method);
    if (found != methods_.end())
    {
        if (found->second.is_request_handler() && !id)
        {
            LOG_WARNING("Missing request id for method:" + method);
            send_telemetry_error("call_method/missing_id");
            return;
        }
        try
        {
            if (found->second.telemetry_level == telemetry_log_level::NO_TELEMETRY)
                method_inflight = {};
            else
                method_inflight = { found->first, std::chrono::steady_clock::now() };

            if (found->second.is_request_handler())
                found->second.as_request_handler()(*id, args);
            if (found->second.is_notification_handler())
                found->second.as_notification_handler()(args);

            telemetry_request_done(method_inflight);
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
        if (id)
            respond_error(id.value(), method, -32601, "MethodNotFound", {});
    }
    else
    {
        std::ostringstream ss;
        ss << "Method " << method << " is not available on this server.";
        LOG_WARNING(ss.str());

        send_telemetry_error("method_not_implemented", method);

        if (id)
            respond_error(id.value(), method, -32601, "MethodNotFound", {});
    }
}

void server::send_telemetry_error(std::string_view where, std::string_view what)
{
    if (!telemetry_provider_)
        return;

    telemetry_provider_->send_telemetry(telemetry_error { where, what });
}

void server::telemetry_request_done(method_telemetry_data start)
{
    if (telemetry_provider_ && !start.method_name.empty())
        telemetry_provider_->send_telemetry(telemetry_info {
            start.method_name,
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start.start).count(),
            std::nullopt,
        });
}

void server::cancel_request_handler(const nlohmann::json& args)
{
    auto cancel_id = args.find("id");
    std::optional<request_id> cid;
    if (cancel_id == args.end() || !cancel_id->get_to(cid))
    {
        LOG_WARNING("Missing id to cancel");
        send_telemetry_error("call_method/missing_cancel_id");
    }
    else if (auto req = cancellable_requests_.extract(*cid); req)
        req.mapped().first();
}

bool server::is_exit_notification_received() const { return exit_notification_received_; }

void server::set_send_message_provider(send_message_provider* provider) { send_message_ = provider; }

bool server::is_shutdown_request_received() const { return shutdown_request_received_; }

} // namespace hlasm_plugin::language_server
