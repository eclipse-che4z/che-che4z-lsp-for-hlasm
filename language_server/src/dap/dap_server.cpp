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

namespace hlasm_plugin::language_server::dap {

server::server(parser_library::workspace_manager& ws_mngr)
    : language_server::server(ws_mngr)
{
    features_.push_back(std::make_unique<dap_feature>(ws_mngr_, *this, this));
    register_feature_methods();
}

void server::request(const json&, const std::string&, const json&, method)
{
    // Currently, there are no supported DAP requests from client to server
    /*send_message_->reply(json {
        { "seq", request_seq }, { "type", "request" }, { "command", requested_command }, { "arguments", args } });*/
}

void server::respond(const json& request_seq, const std::string& requested_command, const json& args)
{
    send_message_->reply(json { { "seq", ++last_seq_ },
        { "type", "response" },
        { "request_seq", request_seq },
        { "success", true },
        { "command", requested_command },
        { "body", args } });
}

void server::notify(const std::string& method, const json& args)
{
    send_message_->reply(json { { "seq", ++last_seq_ }, { "type", "event" }, { "event", method }, { "body", args } });
}

void server::respond_error(const json& request_seq,
    const std::string& requested_command,
    int,
    const std::string& err_message,
    const json& error)
{
    send_message_->reply(json { { "seq", ++last_seq_ },
        { "type", "response" },
        { "request_seq", request_seq },
        { "success", false },
        { "command", requested_command },
        { "message", err_message },
        { "body", json { { "error", error } } } });
}

void server::message_received(const json& message)
{
    try
    {
        if (message.at("type") != "request")
        {
            LOG_WARNING(std::string("Invalid message receive: ") + message.dump());
            return;
        }
        last_seq_ = message.at("seq").get<json::number_unsigned_t>();
        auto arguments = message.find("arguments");
        if (arguments == message.end())
            call_method(message.at("command").get<std::string>(), message.at("seq"), json());
        else
            call_method(message.at("command").get<std::string>(), message.at("seq"), arguments.value());
    }
    catch (const nlohmann::json::exception& e)
    {
        (void)e;
        LOG_WARNING(std::string("There was an error with received request:") + e.what());
    }
}

void server::disconnected()
{
    shutdown_request_received_ = true;
    exit_notification_received_ = true;
}

} // namespace hlasm_plugin::language_server::dap
