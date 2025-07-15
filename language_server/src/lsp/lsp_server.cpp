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

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

#include "../logger.h"
#include "diagnostic.h"
#include "fade_messages.h"
#include "feature_language_features.h"
#include "feature_text_synchronization.h"
#include "feature_workspace_folders.h"
#include "nlohmann/json.hpp"
#include "parsing_metadata_serialization.h"
#include "utils/error_codes.h"
#include "utils/general_hashers.h"
#include "utils/scope_exit.h"
#include "watchers.h"

namespace hlasm_plugin::language_server::lsp {

server::server(parser_library::workspace_manager& ws_mngr)
    : language_server::server(this)
    , ws_mngr(ws_mngr)
    , progress(*this)
{
    features_.push_back(std::make_unique<feature_workspace_folders>(ws_mngr, *this));
    features_.push_back(std::make_unique<feature_text_synchronization>(ws_mngr, *this));
    features_.push_back(std::make_unique<feature_language_features>(ws_mngr, *this));
    register_feature_methods();
    register_methods();

    ws_mngr.register_diagnostics_consumer(this);
    ws_mngr.set_message_consumer(this);
    ws_mngr.set_request_interface(this);
    ws_mngr.set_watcher_registration_provider(this);

    ws_mngr.register_parsing_metadata_consumer(this);
}

void server::consume_parsing_metadata(
    std::string_view, double duration, const parser_library::parsing_metadata& metadata)
{
    if (!telemetry_provider_)
        return;

    telemetry_provider_->send_telemetry(telemetry_info {
        "parsing",
        duration,
        telemetry_metrics_info { metadata },
    });
}

void server::outputs_changed(std::string_view uri)
{
    notify("$/retrieve_outputs",
        nlohmann::json {
            { "uri", uri },
        });
}

namespace {
constexpr std::pair<int, const char*> unknown_error { -1, "Unknown error" };
std::pair<int, const char*> extract_lsp_error(const nlohmann::json& errmsg) noexcept
{
    if (!errmsg.is_object())
        return unknown_error;

    auto code = errmsg.find("code");
    auto msg = errmsg.find("message");

    if (code == errmsg.end() || msg == errmsg.end() || !code->is_number_integer() || !msg->is_string())
        return unknown_error;

    return { code->get<int>(), msg->get<const std::string*>()->c_str() };
}
} // namespace

void server::message_received(const nlohmann::json& message)
{
    std::optional<request_id> id;
    if (auto id_found = message.find("id"); id_found != message.end() && !id_found->get_to(id))
    {
        LOG_WARNING("Invalid id field received.");
        send_telemetry_error("lsp_server/invliad_id");
        return;
    }

    if (auto result_found = message.find("result"); result_found != message.end())
    {
        // we received a response to our request that was successful
        if (!id)
        {
            LOG_WARNING("A response with no id field received.");
            send_telemetry_error("lsp_server/response_no_id");
        }
        else if (auto handler = request_handlers_.extract(*id))
        {
            handler.mapped().first(result_found.value());
        }
        else
        {
            LOG_WARNING("A response with no registered handler received.");
            send_telemetry_error("lsp_server/response_no_handler");
        }
    }
    else if (auto error_result_found = message.find("error"); error_result_found != message.end())
    {
        decltype(request_handlers_.extract(*id)) handler;
        if (id)
            handler = request_handlers_.extract(*id);
        if (handler)
        {
            auto [err, msg] = extract_lsp_error(*error_result_found);
            handler.mapped().second(err, msg);
        }
        else
        {
            std::string warn_message;
            if (auto message_found = error_result_found->find("message"); message_found != error_result_found->end())
                warn_message = message_found->dump();
            else
                warn_message =
                    "Request with id " + (id ? id->to_string() : "<null>") + " returned with unspecified error.";
            LOG_WARNING(warn_message);
            send_telemetry_error("lsp_server/response_error_returned", warn_message);
        }
    }
    else if (auto method_found = message.find("method"); method_found == message.end())
    {
        LOG_WARNING("Method missing from received request or notification");
        send_telemetry_error("lsp_server/method_missing");
    }
    else
    {
        try
        {
            static const nlohmann::json empty;

            const auto& method = method_found.value().get_ref<const std::string&>();
            const auto params_found = message.find("params");
            call_method(method, std::move(id), params_found == message.end() ? empty : params_found.value());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Request failed: ", e.what());
            send_telemetry_error("lsp_server/method_unknown_error");
            if (id)
                respond_error(id.value(),
                    "",
                    -32803,
                    "RequestFailed",
                    {
                        { "method", method_found.value() },
                        { "exception", e.what() },
                    });
        }
    }
}

void server::request(std::string_view requested_method,
    nlohmann::json&& args,
    std::function<void(const nlohmann::json& params)> handler,
    std::function<void(int, const char*)> error_handler)
{
    auto id = request_id_counter++;
    nlohmann::json reply {
        { "jsonrpc", "2.0" },
        { "id", id },
        { "method", requested_method },
        { "params", std::move(args) },
    };
    request_handlers_.try_emplace(request_id(id), std::move(handler), std::move(error_handler));
    send_message_->reply(std::move(reply));
}

void server::respond(const request_id& id, std::string_view, nlohmann::json&& args)
{
    if (auto node = cancellable_requests_.extract(id))
        telemetry_request_done(node.mapped().second);

    nlohmann::json reply {
        { "jsonrpc", "2.0" },
        { "id", id },
        { "result", std::move(args) },
    };
    send_message_->reply(std::move(reply));
}

void server::notify(std::string_view method, nlohmann::json&& args)
{
    nlohmann::json reply {
        { "jsonrpc", "2.0" },
        { "method", method },
        { "params", std::move(args) },
    };
    send_message_->reply(std::move(reply));
}

void server::respond_error(
    const request_id& id, std::string_view, int err_code, std::string_view err_message, nlohmann::json&& error)
{
    cancellable_requests_.erase(id);
    nlohmann::json reply {
        { "jsonrpc", "2.0" },
        { "id", id },
        {
            "error",
            {
                { "code", err_code },
                { "message", err_message },
                { "data", std::move(error) },
            },
        },
    };
    send_message_->reply(std::move(reply));
}

void server::register_cancellable_request(const request_id& id, request_invalidator cancel_handler)
{
    if (cancel_handler)
        cancellable_requests_.try_emplace(id, cancel_handler.take_invalidator(), std::exchange(method_inflight, {}));
}

void server::register_methods()
{
    const auto add_method =
        [this](std::string_view name, auto func, telemetry_log_level telem = telemetry_log_level::NO_TELEMETRY) {
            methods_.try_emplace(std::string(name), method { std::bind_front(func, this), telem });
        };
    add_method("initialize", &server::on_initialize, telemetry_log_level::LOG_EVENT);
    add_method("initialized", &server::on_initialized);
    add_method("shutdown", &server::on_shutdown);
    add_method("exit", &server::on_exit);
    add_method("$/cancelRequest", &server::cancel_request_handler);
    add_method("invalidate_external_configuration", &server::invalidate_external_configuration);
    add_method("toggle_advisory_configuration_diagnostics", &server::toggle_advisory_configuration_diagnostics);
    add_method("set_log_level", &server::set_log_level);
}

void server::send_telemetry(const telemetry_message& message) { notify("telemetry/event", nlohmann::json(message)); }

void empty_handler(const nlohmann::json&)
{
    // Does nothing
}
void empty_error_handler(int, const char*)
{
    // Does nothing
}

void server::on_initialize(const request_id& id, const nlohmann::json& param)
{
    // send server capabilities back
    auto capabilities = nlohmann::json {
        {
            "capabilities",
            {
                { "documentFormattingProvider", false },
                { "documentRangeFormattingProvider", false },
                { "codeActionProvider", false },
                // { "signatureHelpProvider", false },
                { "documentHighlightProvider", false },
                { "renameProvider", false },
                { "workspaceSymbolProvider", false },
            },
        },
    };

    for (auto& f : features_)
    {
        auto feature_cap = f->register_capabilities();
        capabilities["capabilities"].insert(feature_cap.begin(), feature_cap.end());
    }

    respond(id, "", std::move(capabilities));

    nlohmann::json register_configuration_changed_args {
        {
            "registrations",
            {
                {
                    { "id", "configureRegister" },
                    { "method", "workspace/didChangeConfiguration" },
                },
            },
        },
    };

    request("client/registerCapability",
        std::move(register_configuration_changed_args),
        &empty_handler,
        &empty_error_handler);

    for (auto& f : features_)
    {
        f->initialize_feature(param);
    }

    if (progress_notification::client_supports_work_done_progress(param))
        ws_mngr.set_progress_notification_consumer(&progress);

    fill_change_notification_support_flags(param);
}

void server::fill_change_notification_support_flags(const nlohmann::json& initialize_params)
{
    const auto& capabs = initialize_params.at("capabilities");

    if (auto ws = capabs.find("workspace"); ws != capabs.end())
    {
        if (auto watcher = ws->find("didChangeWatchedFiles"); watcher != ws->end() && watcher->is_object())
        {
            m_supports_dynamic_file_change_notification = watcher->value("dynamicRegistration", false);
            m_supports_file_change_notification_relative_pattern = watcher->value("relativePatternSupport", false);
        }
    }
}

void server::on_initialized(const nlohmann::json&)
{
    for (const auto& f : features_)
        f->initialized();
    if (m_supports_dynamic_file_change_notification)
        register_default_watcher();
}

void server::on_shutdown(const request_id& id, const nlohmann::json&)
{
    shutdown_request_received_ = true;

    // perform shutdown
    respond(id, "", nlohmann::json());
}

void server::on_exit(const nlohmann::json&) { exit_notification_received_ = true; }

void server::show_message(std::string_view message, parser_library::message_type type)
{
    nlohmann::json m {
        { "type", (int)type },
        { "message", message },
    };
    notify("window/showMessage", std::move(m));
}

nlohmann::json diagnostic_related_info_to_json(const parser_library::diagnostic& diag)
{
    nlohmann::json related;
    for (const auto& rel : diag.related)
    {
        related.push_back(nlohmann::json {
            {
                "location",
                {
                    { "uri", rel.location.uri },
                    { "range", feature::range_to_json(rel.location.rang) },
                },
            },
            { "message", rel.message },
        });
    }
    return related;
}

namespace {
std::string_view replace_empty_by_space(std::string_view s)
{
    if (s.empty())
        return " ";
    else
        return s;
}

nlohmann::json create_diag_json(const parser_library::range& r,
    std::string_view code,
    std::string_view source,
    std::string_view message,
    std::optional<nlohmann::json> diag_related_info,
    parser_library::diagnostic_severity severity,
    parser_library::diagnostic_tag tags)
{
    nlohmann::json one_json {
        { "range", feature::range_to_json(r) },
        { "code", code },
        { "source", source },
        { "message", replace_empty_by_space(message) },
    };

    if (diag_related_info.has_value())
        one_json["relatedInformation"] = std::move(*diag_related_info);

    if (severity != parser_library::diagnostic_severity::unspecified)
        one_json["severity"] = static_cast<int>(severity);

    if (tags != parser_library::diagnostic_tag::none)
    {
        auto& j_tags = one_json["tags"] = nlohmann::json::array();
        if (static_cast<int>(tags) & static_cast<int>(parser_library::diagnostic_tag::unnecessary))
            j_tags.push_back(1);
        if (static_cast<int>(tags) & static_cast<int>(parser_library::diagnostic_tag::deprecated))
            j_tags.push_back(2);
    }

    return one_json;
}
} // namespace

void server::consume_diagnostics(std::span<const parser_library::diagnostic> diagnostics,
    std::span<const parser_library::fade_message> fade_messages)
{
    std::unordered_map<std::string, nlohmann::json::array_t, utils::hashers::string_hasher, std::equal_to<>> diag_jsons;

    for (const auto& d : diagnostics)
    {
        diag_jsons[d.file_uri].emplace_back(create_diag_json(
            d.diag_range, d.code, d.source, d.message, diagnostic_related_info_to_json(d), d.severity, d.tag));
    }

    for (const auto& fm : fade_messages)
    {
        diag_jsons[fm.uri].emplace_back(create_diag_json(fm.r,
            fm.code,
            fm.source,
            fm.message,
            std::nullopt,
            parser_library::diagnostic_severity::hint,
            parser_library::diagnostic_tag::unnecessary));
    }

    // set of all files for which diagnostics came from the server.
    std::unordered_set<std::string> new_files;
    // transform the diagnostics into json
    for (auto& [uri, diag_json] : diag_jsons)
    {
        new_files.insert(uri);
        last_diagnostics_files_.erase(uri);

        nlohmann::json publish_diags_params {
            { "uri", uri },
            { "diagnostics", std::move(diag_json) },
        };
        notify("textDocument/publishDiagnostics", std::move(publish_diags_params));
    }

    // for each file that had at least one diagnostic in the previous call of this function,
    // but does not have any diagnostics in this call, we send empty diagnostics array to
    // remove the diags from UI
    for (auto& it : last_diagnostics_files_)
    {
        nlohmann::json publish_diags_params {
            { "uri", it },
            { "diagnostics", nlohmann::json::array() },
        };
        notify("textDocument/publishDiagnostics", std::move(publish_diags_params));
    }

    last_diagnostics_files_ = std::move(new_files);
}

void server::request_workspace_configuration(
    std::string_view url, parser_library::workspace_manager_response<std::string_view> json_text)
{
    request(
        "workspace/configuration",
        nlohmann::json { {
            "items",
            nlohmann::json::array_t {
                { { "scopeUri", url } },
            },
        } },
        [json_text](const nlohmann::json& params) {
            if (!params.is_array() || params.size() != 1)
                json_text.error(utils::error::invalid_conf_response);
            else
                json_text.provide(params.at(0).dump());
        },
        [json_text](int err, const char* msg) { json_text.error(err, msg); });
}

void server::request_file_configuration(
    std::string_view uri, parser_library::workspace_manager_response<std::string_view> json_text)
{
    request(
        "external_configuration_request",
        nlohmann::json {
            { "uri", uri },
        },
        [json_text](const nlohmann::json& params) {
            if (!params.is_object() || !params.contains("configuration"))
                json_text.error(utils::error::invalid_external_configuration);
            else
                json_text.provide(params.at("configuration").dump());
        },
        [json_text](int err, const char* msg) { json_text.error(err, msg); });
}

void server::invalidate_external_configuration(const nlohmann::json& data)
{
    auto uri = data.find("uri");
    if (uri != data.end() && uri->is_string())
        ws_mngr.invalidate_external_configuration(uri->get<std::string_view>());
    else
        ws_mngr.invalidate_external_configuration({});
}

void server::toggle_advisory_configuration_diagnostics(const nlohmann::json&)
{
    ws_mngr.toggle_advisory_configuration_diagnostics();
}

void server::set_log_level(const nlohmann::json& data) { logger::instance.level(data.at("log-level").get<unsigned>()); }

parser_library::watcher_registration_id server::add_watcher(std::string_view uri, bool r)
{
    if (!m_supports_file_change_notification_relative_pattern || !m_supports_dynamic_file_change_notification
        || shutdown_request_received_)
        return parser_library::watcher_registration_id::INVALID;

    const auto matching_registration = [uri, r](const auto& w) { return w.base_uri == uri && w.recursive == r; };
    if (const auto it = std::ranges::find_if(m_watcher_registrations, matching_registration);
        it != std::ranges::end(m_watcher_registrations))
    {
        it->reference_count++;
        return it->id;
    }

    const auto id = next_watcher_id();

    m_watcher_registrations.emplace_back(std::string(uri), r, id);

    bool done = false;
    utils::scope_exit remove_on_failure([this, id, &done]() noexcept {
        if (!done)
            std::erase_if(m_watcher_registrations, [id](const watcher_registration& wr) { return wr.id == id; });
    });

    request("client/registerCapability",
        { { "registrations", nlohmann::json::array_t { watcher_registeration(id, uri, r) } } },
        &empty_handler,
        [this, id](int, const char* msg) {
            std::erase_if(m_watcher_registrations, [id](const watcher_registration& wr) { return wr.id == id; });
            LOG_WARNING("Error occurred while registering a file watcher: ", msg);
        });

    done = true;

    return id;
}

void server::remove_watcher(parser_library::watcher_registration_id id)
{
    const auto it = std::ranges::find(m_watcher_registrations, id, &watcher_registration::id);
    if (it == std::ranges::end(m_watcher_registrations) || --it->reference_count > 0)
        return;

    std::swap(*it, m_watcher_registrations.back());
    m_watcher_registrations.pop_back();

    if (shutdown_request_received_)
        return;

    request("client/unregisterCapability",
        { { "unregisterations", nlohmann::json::array_t { watcher_unregisteration(id) } } },
        &empty_handler,
        [](int, const char* msg) { LOG_WARNING("Error occurred while unregistering file watcher: ", msg); });
}

void server::register_default_watcher()
{
    request("client/registerCapability",
        { { "registrations", nlohmann::json::array_t { default_watcher_registration() } } },
        &empty_handler,
        [](int, const char* msg) { LOG_WARNING("Error occurred while registering global file watcher: ", msg); });
}

parser_library::watcher_registration_id server::next_watcher_id() noexcept
{
    const auto n = static_cast<std::underlying_type_t<parser_library::watcher_registration_id>>(m_next_watcher_id) + 1;
    const auto w = static_cast<parser_library::watcher_registration_id>(n);
    m_next_watcher_id = w;
    return w;
}

void server::testing_enable_capabilities()
{
    m_supports_dynamic_file_change_notification = true;
    m_supports_file_change_notification_relative_pattern = true;
}
} // namespace hlasm_plugin::language_server::lsp
