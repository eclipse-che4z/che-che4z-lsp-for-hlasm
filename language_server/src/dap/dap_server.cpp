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


#include "dap_server.h"

#include "../logger.h"
#include "dap_feature.h"
#include "nlohmann/json.hpp"

namespace hlasm_plugin::language_server::dap {

server::server(parser_library::debugger_configuration_provider& dc_provider, telemetry_sink* telemetry_reporter)
    : language_server::server(telemetry_reporter)
{
    auto dap_f = std::make_unique<dap_feature>(dc_provider, *this, this);
    m_dap_feature = dap_f.get();
    features_.push_back(std::move(dap_f));
    register_feature_methods();
}

void server::request(const std::string&, const nlohmann::json&, std::function<void(const nlohmann::json& params)>)
{
    // Currently, there are no supported DAP requests from client to server
    /*send_message_->reply(nlohmann::json {
        { "seq", request_seq }, { "type", "request" }, { "command", requested_command }, { "arguments", args } });*/
}

void server::respond(const request_id& request_seq, const std::string& requested_command, const nlohmann::json& args)
{
    send_message_->reply(nlohmann::json {
        { "seq", ++last_seq_ },
        { "type", "response" },
        { "request_seq", request_seq },
        { "success", true },
        { "command", requested_command },
        { "body", args },
    });
}

void server::notify(const std::string& method, const nlohmann::json& args)
{
    send_message_->reply(nlohmann::json {
        { "seq", ++last_seq_ },
        { "type", "event" },
        { "event", method },
        { "body", args },
    });
}

void server::respond_error(const request_id& request_seq,
    const std::string& requested_command,
    int,
    const std::string& err_message,
    const nlohmann::json& error)
{
    send_message_->reply(nlohmann::json {
        { "seq", ++last_seq_ },
        { "type", "response" },
        { "request_seq", request_seq },
        { "success", false },
        { "command", requested_command },
        { "message", err_message },
        { "body", { { "error", error } } },
    });
}

void server::register_cancellable_request(const request_id&, request_invalidator)
{ /* not supported in dap */
}

void server::message_received(const nlohmann::json& message)
{
    try
    {
        if (message.at("type") != "request")
        {
            LOG_WARNING(std::string("Invalid message receive: ") + message.dump());
            send_telemetry_error("dap_server/invalid_message");
            return;
        }
        call_method(message.at("command").get<std::string>(),
            message.at("seq").get<request_id>(),
            message.value("arguments", nlohmann::json()));
    }
    catch (const nlohmann::json::exception& e)
    {
        (void)e;
        LOG_WARNING(std::string("There was an error with received request:") + e.what());
        send_telemetry_error("dap_server/method_unknown_error");
    }
}

void server::disconnected()
{
    shutdown_request_received_ = true;
    exit_notification_received_ = true;
}


void server::idle_handler(const std::atomic<unsigned char>* yield_indicator)
{
    m_dap_feature->idle_handler(yield_indicator);
}

} // namespace hlasm_plugin::language_server::dap
