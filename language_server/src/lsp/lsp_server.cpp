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


#include "lsp_server.h"

#include <functional>
#include <map>

#include "../logger.h"
#include "feature_language_features.h"
#include "feature_text_synchronization.h"
#include "feature_workspace_folders.h"
#include "lib_config.h"

namespace hlasm_plugin::language_server::lsp {

server::server(parser_library::workspace_manager& ws_mngr)
    : language_server::server(ws_mngr)
{
    features_.push_back(std::make_unique<feature_workspace_folders>(ws_mngr_, *this));
    features_.push_back(std::make_unique<feature_text_synchronization>(ws_mngr_, *this));
    features_.push_back(std::make_unique<feature_language_features>(ws_mngr_, *this));
    register_feature_methods();
    register_methods();

    ws_mngr_.register_diagnostics_consumer(this);
    ws_mngr_.set_message_consumer(this);
}

void server::message_received(const json& message)
{
    auto id_found = message.find("id");


    auto result_found = message.find("result");
    auto error_result_found = message.find("error");

    if (result_found != message.end())
    {
        // we received a response to our request that was successful
        if (id_found == message.end())
        {
            LOG_WARNING("A response with no id field received.");
            return;
        }

        auto handler_found = request_handlers_.find(*id_found);
        if (handler_found == request_handlers_.end())
        {
            LOG_WARNING("A response with no registered handler received.");
            return;
        }

        method handler = handler_found->second;
        request_handlers_.erase(handler_found);
        handler(id_found.value(), result_found.value());
        return;
    }
    else if (error_result_found != message.end())
    {
        auto message_found = error_result_found->find("message");
        std::string warn_message;
        if (message_found != error_result_found->end())
            warn_message = message_found->dump();
        else
            warn_message = "Request with id " + id_found->dump() + " returned with unspecified error.";
        LOG_WARNING(warn_message);
        return;
    }

    auto params_found = message.find("params");
    auto method_found = message.find("method");

    if (params_found == message.end() || method_found == message.end())
    {
        LOG_WARNING("Method or params missing from received request or notification");
        return;
    }

    if (id_found == message.end())
    {
        // notification
        call_method(method_found.value().get<std::string>(), "", params_found.value());
    }
    else
    {
        // method
        call_method(method_found.value().get<std::string>(), id_found.value(), params_found.value());
    }
}

void server::request(const json& id, const std::string& requested_method, const json& args, method handler)
{
    json reply { { "jsonrpc", "2.0" }, { "id", id }, { "method", requested_method }, { "params", args } };
    request_handlers_.emplace(id, handler);
    send_message_->reply(reply);
}

void server::respond(const json& id, const std::string&, const json& args)
{
    json reply { { "jsonrpc", "2.0" }, { "id", id }, { "result", args } };
    send_message_->reply(reply);
}

void server::notify(const std::string& method, const json& args)
{
    json reply { { "jsonrpc", "2.0" }, { "method", method }, { "params", args } };
    send_message_->reply(reply);
}

void server::respond_error(
    const json& id, const std::string&, int err_code, const std::string& err_message, const json& error)
{
    json reply { { "jsonrpc", "2.0" },
        { "id", id },
        { "error", json { { "code", err_code }, { "message", err_message }, { "data", error } } } };
    send_message_->reply(reply);
}

void server::register_methods()
{
    methods_.emplace(
        "initialize", std::bind(&server::on_initialize, this, std::placeholders::_1, std::placeholders::_2));
    methods_.emplace("shutdown", std::bind(&server::on_shutdown, this, std::placeholders::_1, std::placeholders::_2));
    methods_.emplace("exit", std::bind(&server::on_exit, this, std::placeholders::_1, std::placeholders::_2));
}

void empty_handler(json, const json&)
{
    // Does nothing
}

void server::on_initialize(json id, const json& param)
{
    // send server capabilities back
    json capabilities = json { { "capabilities",
        json { { "documentFormattingProvider", false },
            { "documentRangeFormattingProvider", false },
            { "codeActionProvider", false },
            { "signatureHelpProvider",
                json {
                    { "triggerCharacters", { "(", "," } },
                } },
            { "documentHighlightProvider", false },
            { "renameProvider", false },
            { "documentSymbolProvider", false },
            { "workspaceSymbolProvider", false },
            { "semanticHighlighting", true } } } };

    for (auto& f : features_)
    {
        json feature_cap = f->register_capabilities();
        capabilities["capabilities"].insert(feature_cap.begin(), feature_cap.end());
    }

    respond(id, "", capabilities);

    json register_configuration_changed_args {
        { { "registrations", { { { "id", "configureRegister" }, { "method", "workspace/didChangeConfiguration" } } } } }
    };
    /*json register_message{ { "jsonrpc", "2.0" },
    { "id", "register1" },
    { "method", "client/registerCapability" },
    { "params",  } };
    send_message_->reply(register_message);*/

    request("register1", "client/registerCapability", register_configuration_changed_args, &empty_handler);


    for (auto& f : features_)
    {
        f->initialize_feature(param);
    }
}

void server::on_shutdown(json id, const json&)
{
    shutdown_request_received_ = true;

    // perform shutdown
    json rep = json {};
    respond(id, "", rep);
}

void server::on_exit(json, const json&) { exit_notification_received_ = true; }

void server::show_message(const std::string& message, parser_library::message_type type)
{
    json m { { "type", (int)type }, { "message", message } };
    notify("window/showMessage", m);
}

json diagnostic_related_info_to_json(parser_library::diagnostic& diag)
{
    json related;
    for (size_t i = 0; i < diag.related_info_size(); ++i)
    {
        related.push_back(
            json { { "location",
                       json { { "uri", feature::path_to_uri(diag.related_info(i).location().uri()) },
                           { "range", feature::range_to_json(diag.related_info(i).location().get_range()) } } },
                { "message", diag.related_info(i).message() } });
    }
    return related;
}

void server::consume_diagnostics(parser_library::diagnostic_list diagnostics)
{
    // map of all diagnostics that came from the server
    std::map<std::string, std::vector<parser_library::diagnostic>> diags;

    for (size_t i = 0; i < diagnostics.diagnostics_size(); ++i)
    {
        auto d = diagnostics.diagnostics(i);
        diags[d.file_name()].push_back(d);
    }

    // set of all files for which diagnostics came from the server.
    std::unordered_set<std::string> new_files;
    // transform the diagnostics into json
    for (const auto& file_diags : diags)
    {
        json diags_array = json::array();
        for (auto d : file_diags.second)
        {
            json one_json { { "range", feature::range_to_json(d.get_range()) },
                { "code", d.code() },
                { "source", d.source() },
                { "message", d.message() },
                { "relatedInformation", diagnostic_related_info_to_json(d) } };
            if (d.severity() != parser_library::diagnostic_severity::unspecified)
            {
                one_json["severity"] = (int)d.severity();
            }
            diags_array.push_back(std::move(one_json));
        }

        json publish_diags_params { { "uri", feature::path_to_uri(file_diags.first) }, { "diagnostics", diags_array } };
        new_files.insert(file_diags.first);
        last_diagnostics_files_.erase(file_diags.first);

        notify("textDocument/publishDiagnostics", publish_diags_params);
    }

    // for each file that had at least one diagnostic in the previous call of this function,
    // but does not have any diagnostics in this call, we send empty diagnostics array to
    // remove the diags from UI
    for (auto& it : last_diagnostics_files_)
    {
        json publish_diags_params { { "uri", feature::path_to_uri(it) }, { "diagnostics", json::array() } };
        notify("textDocument/publishDiagnostics", publish_diags_params);
    }

    last_diagnostics_files_ = std::move(new_files);
}


} // namespace hlasm_plugin::language_server::lsp
