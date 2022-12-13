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
#include "parsing_metadata_serialization.h"
#include "utils/resource_location.h"

namespace hlasm_plugin::language_server::lsp {

server::server(parser_library::workspace_manager& ws_mngr)
    : language_server::server(ws_mngr, this)
{
    features_.push_back(std::make_unique<feature_workspace_folders>(ws_mngr_, *this));
    features_.push_back(std::make_unique<feature_text_synchronization>(ws_mngr_, *this));
    features_.push_back(std::make_unique<feature_language_features>(ws_mngr_, *this));
    register_feature_methods();
    register_methods();

    ws_mngr_.register_diagnostics_consumer(this);
    ws_mngr_.set_message_consumer(this);

    ws_mngr_.register_parsing_metadata_consumer(&parsing_metadata_);
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
            send_telemetry_error("lsp_server/response_no_id");
            return;
        }

        auto handler_found = request_handlers_.find(*id_found);
        if (handler_found == request_handlers_.end())
        {
            LOG_WARNING("A response with no registered handler received.");
            send_telemetry_error("lsp_server/response_no_handler");
            return;
        }

        method handler = handler_found->second;
        request_handlers_.erase(handler_found);
        handler.handler(id_found.value(), result_found.value());
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
        send_telemetry_error("lsp_server/response_error_returned", warn_message);
        return;
    }

    auto params_found = message.find("params");
    auto method_found = message.find("method");

    if (method_found == message.end())
    {
        LOG_WARNING("Method missing from received request or notification");
        send_telemetry_error("lsp_server/method_missing");
        return;
    }

    try
    {
        call_method(method_found.value().get<std::string>(),
            id_found == message.end() ? nlohmann::json() : id_found.value(),
            params_found == message.end() ? nlohmann::json() : params_found.value());
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(e.what());
        send_telemetry_error("lsp_server/method_unknown_error");
        return;
    }
}

telemetry_metrics_info server::get_telemetry_details()
{
    return telemetry_metrics_info { parsing_metadata_.data, diags_error_count, diags_warning_count };
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
    methods_.try_emplace("initialize",
        method { [this](const json& id, const json& params) { on_initialize(id, params); },
            telemetry_log_level::LOG_EVENT });
    methods_.try_emplace("initialized",
        method { [](const json&, const json&) { /*no implementation, silences uninteresting telemetry*/ },
            telemetry_log_level::NO_TELEMETRY });
    methods_.try_emplace("shutdown",
        method { [this](const json& id, const json& params) { on_shutdown(id, params); },
            telemetry_log_level::NO_TELEMETRY });
    methods_.try_emplace("exit",
        method {
            [this](const json& id, const json& params) { on_exit(id, params); }, telemetry_log_level::NO_TELEMETRY });
    methods_.try_emplace("$/cancelRequest",
        method { [](const json&, const json&) {
                    /*no implementation, silences telemetry reporting*/
                },
            telemetry_log_level::NO_TELEMETRY });
}

void server::send_telemetry(const telemetry_message& message) { notify("telemetry/event", json(message)); }

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
            { "signatureHelpProvider", false },
            { "documentHighlightProvider", false },
            { "renameProvider", false },
            { "workspaceSymbolProvider", false } } } };

    for (auto& f : features_)
    {
        json feature_cap = f->register_capabilities();
        capabilities["capabilities"].insert(feature_cap.begin(), feature_cap.end());
    }

    respond(id, "", capabilities);

    json register_configuration_changed_args {
        { { "registrations", { { { "id", "configureRegister" }, { "method", "workspace/didChangeConfiguration" } } } } }
    };

    request("register1",
        "client/registerCapability",
        register_configuration_changed_args,
        { &empty_handler, telemetry_log_level::NO_TELEMETRY });


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
                       json { { "uri", diag.related_info(i).location().uri() },
                           { "range", feature::range_to_json(diag.related_info(i).location().get_range()) } } },
                { "message", diag.related_info(i).message() } });
    }
    return related;
}

namespace {
std::string replace_empty_by_space(std::string s)
{
    if (s.empty())
        return " ";
    else
        return s;
}
} // namespace

void server::consume_diagnostics(parser_library::diagnostic_list diagnostics)
{
    // map of all diagnostics that came from the server
    std::map<std::string, std::vector<parser_library::diagnostic>> diags;

    diags_error_count = 0;
    diags_warning_count = 0;

    for (size_t i = 0; i < diagnostics.diagnostics_size(); ++i)
    {
        auto d = diagnostics.diagnostics(i);
        diags[d.file_uri()].push_back(d);
        if (d.severity() == parser_library::diagnostic_severity::error)
            ++diags_error_count;
        else if (d.severity() == parser_library::diagnostic_severity::warning)
            ++diags_warning_count;
    }

    // set of all files for which diagnostics came from the server.
    std::unordered_set<std::string> new_files;
    // transform the diagnostics into json
    for (const auto& file_diags : diags)
    {
        json diags_array = json::array();
        for (auto d : file_diags.second)
        {
            json one_json {
                { "range", feature::range_to_json(d.get_range()) },
                { "code", d.code() },
                { "source", d.source() },
                { "message", replace_empty_by_space(d.message()) },
                { "relatedInformation", diagnostic_related_info_to_json(d) },
            };
            if (d.severity() != parser_library::diagnostic_severity::unspecified)
            {
                one_json["severity"] = (int)d.severity();
            }
            if (auto t = d.tags(); t != parser_library::diagnostic_tag::none)
            {
                auto& tags = one_json["tags"] = json::array();
                if (static_cast<int>(t) & static_cast<int>(parser_library::diagnostic_tag::unnecessary))
                    tags.push_back(1);
                if (static_cast<int>(t) & static_cast<int>(parser_library::diagnostic_tag::deprecated))
                    tags.push_back(2);
            }
            diags_array.push_back(std::move(one_json));
        }

        json publish_diags_params { { "uri", file_diags.first }, { "diagnostics", diags_array } };
        new_files.insert(file_diags.first);
        last_diagnostics_files_.erase(file_diags.first);

        notify("textDocument/publishDiagnostics", publish_diags_params);
    }

    // for each file that had at least one diagnostic in the previous call of this function,
    // but does not have any diagnostics in this call, we send empty diagnostics array to
    // remove the diags from UI
    for (auto& it : last_diagnostics_files_)
    {
        json publish_diags_params { { "uri", it }, { "diagnostics", json::array() } };
        notify("textDocument/publishDiagnostics", publish_diags_params);
    }

    last_diagnostics_files_ = std::move(new_files);
}


} // namespace hlasm_plugin::language_server::lsp
