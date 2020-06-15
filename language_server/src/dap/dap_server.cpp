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

#include <functional>
#include <map>

#include "../logger.h"
#include "feature_launch.h"

namespace hlasm_plugin::language_server::dap {

server::server(parser_library::workspace_manager& ws_mngr)
    : language_server::server(ws_mngr)
{
    features_.push_back(std::make_unique<feature_launch>(ws_mngr_, *this));
    register_feature_methods();
    register_methods();
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
        assert(message["type"] == "request");
        last_seq_ = message["seq"].get<json::number_unsigned_t>();
        auto arguments = message.find("arguments");
        if (arguments == message.end())
            call_method(message["command"].get<std::string>(), message["seq"], json());
        else
            call_method(message["command"].get<std::string>(), message["seq"], arguments.value());
    }
    catch (const nlohmann::json::exception& e)
    {
        (void)e;
        LOG_WARNING(std::string("There was an error with received request:") + e.what());
    }
}

void server::register_methods()
{
    methods_.emplace(
        "initialize", std::bind(&server::on_initialize, this, std::placeholders::_1, std::placeholders::_2));
    methods_.emplace(
        "disconnect", std::bind(&server::on_disconnect, this, std::placeholders::_1, std::placeholders::_2));
}

void server::on_initialize(const json& requested_seq, const json& args)
{
    json capabilities = json::object();

    for (auto& f : features_)
    {
        json feature_cap = f->register_capabilities();
        capabilities.insert(feature_cap.begin(), feature_cap.end());
    }

    respond(requested_seq, "initialize", capabilities);


    for (auto& f : features_)
    {
        f->initialize_feature(args);
    }

    notify("initialized", json());
}

void server::on_disconnect(const json& request_seq, const json&)
{
    ws_mngr_.disconnect();

    respond(request_seq, "disconnect", json());

    shutdown_request_received_ = true;
    exit_notification_received_ = true;
}


} // namespace hlasm_plugin::language_server::dap
