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

#include "dap_feature.h"

#include <format>
#include <functional>

#include "nlohmann/json.hpp"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "workspace_manager_response.h"

namespace {
using namespace hlasm_plugin::language_server::dap;

std::string server_conformant_path(std::string_view path, path_format path_format)
{
    // Server accepts paths in URI format
    if (path_format == path_format::URI || hlasm_plugin::utils::path::is_likely_uri(path))
        return std::string(path);

    // Theia sends us relative path while not accepting it back. Change to absolute
    std::filesystem::path p = hlasm_plugin::utils::path::absolute(path);

    std::string result = hlasm_plugin::utils::path::lexically_normal(p).string();

    // On windows, VS code sends us path with capital drive letter through DAP and
    // lowercase drive letter through LSP.
    // Remove, once we implement case-insensitive comparison of paths in parser_library for windows
    if (hlasm_plugin::utils::platform::is_windows())
    {
        if (result[1] == ':')
            result[0] = (char)tolower((unsigned char)result[0]);
    }

    return hlasm_plugin::utils::path::path_to_uri(result);
}

std::string client_conformant_path(std::string_view uri, path_format client_path_format)
{
    // Server provides paths in URI format -> convert it to whatever the client wants
    if (client_path_format == path_format::URI)
        return std::string(uri);

    auto generated_path = hlasm_plugin::utils::path::uri_to_path(uri);

    if (generated_path.empty())
        generated_path = uri;

    return generated_path;
}

constexpr int THREAD_ID = 1;

} // namespace

namespace hlasm_plugin::language_server::dap {
void dap_feature::initialize_feature(const nlohmann::json&) { /* nothing to do */ }

dap_feature::dap_feature(parser_library::debugger_configuration_provider& dc_provider,
    response_provider& response_provider,
    dap_disconnect_listener* disconnect_listener)
    : feature(response_provider)
    , dc_provider(dc_provider)
    , disconnect_listener_(disconnect_listener)
{}


void dap_feature::register_methods(std::map<std::string, method>& methods)
{
    using enum telemetry_log_level;
    const auto add_method = [this, &methods](std::string_view name,
                                auto func,
                                telemetry_log_level telem = telemetry_log_level::NO_TELEMETRY) {
        methods.try_emplace(std::string(name), method { std::bind_front(func, this), telem });
    };
    add_method("initialize", &dap_feature::on_initialize);
    add_method("disconnect", &dap_feature::on_disconnect, LOG_EVENT);
    add_method("launch", &dap_feature::on_launch, LOG_EVENT);
    add_method("setBreakpoints", &dap_feature::on_set_breakpoints, LOG_EVENT);
    add_method("setExceptionBreakpoints", &dap_feature::on_set_exception_breakpoints, LOG_EVENT);
    add_method("setFunctionBreakpoints", &dap_feature::on_set_function_breakpoints, LOG_EVENT);
    add_method("configurationDone", &dap_feature::on_configuration_done);
    add_method("threads", &dap_feature::on_threads);
    add_method("stackTrace", &dap_feature::on_stack_trace);
    add_method("scopes", &dap_feature::on_scopes);
    add_method("next", &dap_feature::on_next, LOG_EVENT);
    add_method("stepIn", &dap_feature::on_step_in, LOG_EVENT);
    add_method("stepOut", &dap_feature::on_step_out, LOG_EVENT);
    add_method("variables", &dap_feature::on_variables);
    add_method("continue", &dap_feature::on_continue, LOG_EVENT);
    add_method("pause", &dap_feature::on_pause, LOG_EVENT);
    add_method("evaluate", &dap_feature::on_evaluate, LOG_EVENT);
}
nlohmann::json dap_feature::register_capabilities() { return nlohmann::json(); }

void dap_feature::stopped(std::string_view reason, std::string_view details)
{
    nlohmann::json args {
        { "reason", reason },
        { "threadId", THREAD_ID },
        { "allThreadsStopped", true },
    };
    if (details.size() > 0)
    {
        args["description"] = details;
        args["text"] = details;
    }

    response_->notify("stopped", std::move(args));
}

void dap_feature::exited(int exit_code)
{
    response_->notify("exited", nlohmann::json { { "exitCode", exit_code } });
    response_->notify("terminated", nlohmann::json());
}

void dap_feature::mnote(unsigned char level, std::string_view text)
{
    response_->notify("output",
        nlohmann::json {
            { "category", "stderr" },
            { "output", std::format("{}:{}\n", level, text) },
        });
}

void dap_feature::punch(std::string_view text)
{
    response_->notify("output",
        nlohmann::json {
            { "category", "stdout" },
            { "output", std::format("{}\n", text) },
        });
}

void dap_feature::on_initialize(const request_id& requested_seq, const nlohmann::json& args)
{
    response_->respond(requested_seq,
        "initialize",
        nlohmann::json {
            { "supportsConfigurationDoneRequest", true },
            { "supportsEvaluateForHovers", true },
            { "supportsFunctionBreakpoints", true },
        });

    line_1_based_ = args.at("linesStartAt1").get<bool>() ? 1 : 0;
    column_1_based_ = args.at("columnsStartAt1").get<bool>() ? 1 : 0;
    client_path_format_ =
        args.at("pathFormat").get<std::string_view>() == "path" ? path_format::PATH : path_format::URI;

    debugger.emplace();

    response_->notify("initialized", nlohmann::json());
}

void dap_feature::on_disconnect(const request_id& request_seq, const nlohmann::json&)
{
    if (disconnect_listener_)
        disconnect_listener_->disconnected();

    debugger.reset();

    response_->respond(request_seq, "disconnect", nlohmann::json());
}

void dap_feature::on_launch(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;

    // wait for configurationDone?
    auto program_path = server_conformant_path(args.at("program").get<std::string_view>(), client_path_format_);
    bool stop_on_entry = args.at("stopOnEntry").get<bool>();
    debugger->set_event_consumer(this);

    struct launch_handler
    {
        request_id rs;
        response_provider* rp;

        void provide(bool launched) const
        {
            if (launched)
                rp->respond(rs, "launch", nlohmann::json());
            else
                rp->respond_error(rs, "launch", 0, "File not found", nlohmann::json());
        }

        void error(int err, const char* msg) const noexcept
        {
            // terminates on throw
            rp->respond_error(rs, "launch", err, msg, nlohmann::json());
        }
    };

    debugger->launch(program_path,
        dc_provider,
        stop_on_entry,
        parser_library::make_workspace_manager_response(launch_handler { request_seq, response_ }).first);
}

void dap_feature::on_set_breakpoints(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;

    nlohmann::json breakpoints_verified = nlohmann::json::array();

    auto source = server_conformant_path(args.at("source").at("path").get<std::string_view>(), client_path_format_);
    std::vector<parser_library::debugging::breakpoint> breakpoints;

    if (auto bpoints_found = args.find("breakpoints"); bpoints_found != args.end())
    {
        for (auto& bp_json : bpoints_found.value())
        {
            breakpoints.emplace_back(bp_json.at("line").get<nlohmann::json::number_unsigned_t>() - line_1_based_);
            breakpoints_verified.push_back(nlohmann::json { { "verified", true } });
        }
    }
    debugger->breakpoints(source, breakpoints);

    response_->respond(request_seq, "setBreakpoints", nlohmann::json { { "breakpoints", breakpoints_verified } });
}

void dap_feature::on_set_exception_breakpoints(const request_id& request_seq, const nlohmann::json&)
{
    response_->respond(request_seq, "setExceptionBreakpoints", nlohmann::json());
}

void dap_feature::on_set_function_breakpoints(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;

    nlohmann::json breakpoints_verified = nlohmann::json::array();
    std::vector<parser_library::debugging::function_breakpoint> breakpoints;

    if (auto bpoints_found = args.find("breakpoints"); bpoints_found != args.end())
    {
        for (auto& bp_json : bpoints_found.value())
        {
            breakpoints.emplace_back(bp_json.at("name").get<std::string_view>());
            breakpoints_verified.push_back(nlohmann::json { { "verified", true } });
        }
    }

    debugger->function_breakpoints(breakpoints);

    response_->respond(
        request_seq, "setFunctionBreakpoints", nlohmann::json { { "breakpoints", breakpoints_verified } });
}

void dap_feature::on_configuration_done(const request_id& request_seq, const nlohmann::json&)
{
    response_->respond(request_seq, "configurationDone", nlohmann::json());
}

void dap_feature::on_threads(const request_id& request_seq, const nlohmann::json&)
{
    response_->respond(request_seq,
        "threads",
        nlohmann::json { {
            "threads",
            nlohmann::json::array({
                {
                    { "id", THREAD_ID },
                    { "name", "main" },
                },
            }),
        } });
}

[[nodiscard]] nlohmann::json source_to_json(const parser_library::debugging::source& source, path_format path_format)
{
    return nlohmann::json { { "path", client_conformant_path(source.uri, path_format) } };
}

void dap_feature::on_stack_trace(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    nlohmann::json frames_json = nlohmann::json::array();

    for (const auto& frame : debugger->stack_frames())
    {
        frames_json.push_back(nlohmann::json {
            { "id", frame.id },
            { "name", frame.name },
            { "source", source_to_json(frame.frame_source, client_path_format_) },
            { "line", frame.begin_line + line_1_based_ },
            { "column", column_1_based_ },
            { "endLine", frame.end_line + line_1_based_ },
            { "endColumn", column_1_based_ },
        });
    }


    response_->respond(request_seq,
        "stackTrace",
        nlohmann::json {
            { "stackFrames", frames_json },
            { "totalFrames", frames_json.size() },
        });
}

void dap_feature::on_scopes(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;

    nlohmann::json scopes_json = nlohmann::json::array();

    for (const auto& scope : debugger->scopes(args.at("frameId").get<nlohmann::json::number_unsigned_t>()))
    {
        auto scope_json = nlohmann::json {
            { "name", scope.name },
            { "variablesReference", scope.var_reference },
            { "expensive", false },
            { "source", source_to_json(scope.scope_source, client_path_format_) },
        };
        scopes_json.push_back(std::move(scope_json));
    }


    response_->respond(request_seq, "scopes", nlohmann::json { { "scopes", scopes_json } });
}

void dap_feature::on_next(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    debugger->next();

    response_->respond(request_seq, "next", nlohmann::json());
}

void dap_feature::on_step_in(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    debugger->step_in();
    response_->respond(request_seq, "stepIn", nlohmann::json());
}

void dap_feature::on_step_out(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    debugger->step_out();
    response_->respond(request_seq, "stepOut", nlohmann::json());
}

void dap_feature::on_variables(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;

    nlohmann::json variables_json = nlohmann::json::array();

    for (const auto& var :
        debugger->variables(parser_library::debugging::var_reference_t(args.at("variablesReference"))))
    {
        using enum parser_library::debugging::set_type;

        std::string type;
        switch (var.type)
        {
            case A_TYPE:
                type = "A_TYPE";
                break;
            case B_TYPE:
                type = "B_TYPE";
                break;
            case C_TYPE:
                type = "C_TYPE";
                break;
            default:
                break;
        }

        nlohmann::json var_json;
        if (type == "")
            var_json = nlohmann::json {
                { "name", var.name },
                { "value", var.value },
                { "variablesReference", var.var_reference },
            };
        else
            var_json = nlohmann::json { { "name", var.name },
                { "value", var.value },
                { "variablesReference", var.var_reference },
                { "type", type } };

        variables_json.push_back(std::move(var_json));
    }

    response_->respond(request_seq, "variables", nlohmann::json { { "variables", variables_json } });
}

void dap_feature::on_continue(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    debugger->continue_debug();

    response_->respond(request_seq, "continue", nlohmann::json { { "allThreadsContinued", true } });
}

void dap_feature::on_pause(const request_id& request_seq, const nlohmann::json&)
{
    if (!debugger)
        return;

    debugger->pause();

    response_->respond(request_seq, "pause", nlohmann::json());
}

void dap_feature::on_evaluate(const request_id& request_seq, const nlohmann::json& args)
{
    if (!debugger)
        return;
    const std::string_view expression = args.at("expression").get<std::string_view>();
    const auto frame_id = args.value("frameId", parser_library::debugging::frame_id_t(-1));

    auto result = debugger->evaluate(expression, frame_id);

    if (result.error)
        response_->respond_error(request_seq, "evaluate", -1, result.result, nlohmann::json());
    else
        response_->respond(request_seq,
            "evaluate",
            nlohmann::json {
                { "result", result.result },
                { "variablesReference", result.var_ref },
            });
}


void dap_feature::idle_handler(const std::atomic<unsigned char>* yield_indicator)
{
    if (debugger)
        debugger->analysis_step(yield_indicator);
}

} // namespace hlasm_plugin::language_server::dap
