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

#include "feature_launch.h"

#include <filesystem>

namespace hlasm_plugin::language_server::dap {

constexpr int THREAD_ID = 1;

feature_launch::feature_launch(parser_library::workspace_manager& ws_mngr, response_provider& response_provider)
    : dap_feature(ws_mngr, response_provider)
{
    ws_mngr_.register_debug_event_consumer(*this);
}

feature_launch::~feature_launch() { ws_mngr_.unregister_debug_event_consumer(*this); }

void feature_launch::register_methods(std::map<std::string, method>& methods)
{
    methods.emplace(
        "launch", std::bind(&feature_launch::on_launch, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace("setBreakpoints",
        std::bind(&feature_launch::on_set_breakpoints, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace("setExceptionBreakpoints",
        std::bind(&feature_launch::on_set_exception_breakpoints, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace("configurationDone",
        std::bind(&feature_launch::on_configuration_done, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "threads", std::bind(&feature_launch::on_threads, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "stackTrace", std::bind(&feature_launch::on_stack_trace, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "scopes", std::bind(&feature_launch::on_scopes, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace("next", std::bind(&feature_launch::on_next, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "stepIn", std::bind(&feature_launch::on_step_in, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "variables", std::bind(&feature_launch::on_variables, this, std::placeholders::_1, std::placeholders::_2));
    methods.emplace(
        "continue", std::bind(&feature_launch::on_continue, this, std::placeholders::_1, std::placeholders::_2));
}

json feature_launch::register_capabilities() { return json { { "supportsConfigurationDoneRequest", true } }; }

void feature_launch::on_launch(const json& request_seq, const json& args)
{
    // wait for configurationDone?
    std::string program_path = convert_path(args["program"].get<std::string>());
    ws_mngr_.launch(program_path.c_str(), args["stopOnEntry"].get<bool>());
    response_->respond(request_seq, "launch", json());
}

void feature_launch::on_set_breakpoints(const json& request_seq, const json& args)
{
    std::string source = convert_path(args["source"]["path"].get<std::string>());

    std::vector<parser_library::breakpoint> breakpoints;

    json breakpoints_verified = json::array();

    auto bpoints_found = args.find("breakpoints");
    if (bpoints_found != args.end())
    {
        for (auto& bp_json : bpoints_found.value())
        {
            breakpoints.emplace_back(bp_json["line"].get<json::number_unsigned_t>() - line_1_based_);
            breakpoints_verified.push_back(json { { "verified", true } });
        }
    }


    ws_mngr_.set_breakpoints(source.c_str(), breakpoints.data(), breakpoints.size());


    response_->respond(request_seq, "setBreakpoints", json { { "breakpoints", breakpoints_verified } });
}

void feature_launch::on_set_exception_breakpoints(const json& request_seq, const json&)
{
    response_->respond(request_seq, "setExceptionBreakpoints", json());
}

void feature_launch::on_configuration_done(const json& request_seq, const json&)
{
    response_->respond(request_seq, "configurationDone", json());
}



void feature_launch::on_threads(const json& request_seq, const json&)
{
    response_->respond(request_seq,
        "threads",
        json { { "threads", json::array({ json { { "id", THREAD_ID }, { "name", "main" } } }) } });
}

json source_to_json(parser_library::source source) { return json { { "path", source.path() } }; }

void feature_launch::on_stack_trace(const json& request_seq, const json&)
{
    parser_library::stack_frames frames = ws_mngr_.get_stack_frames();

    json frames_json = nlohmann::json::array();

    for (size_t i = 0; i < frames.size(); ++i)
    {
        parser_library::stack_frame frame = frames.item(i);

        frames_json.push_back(json {
            { "id", frame.id() },
            { "name", frame.name() },
            { "source", source_to_json(frame.get_source()) },
            { "line", frame.get_range().start.line + line_1_based_ },
            { "column", frame.get_range().start.column + column_1_based_ },
            { "endLine", frame.get_range().end.line + line_1_based_ },
            { "endColumn", frame.get_range().end.column + column_1_based_ },
        });
    }

    response_->respond(
        request_seq, "stackTrace", json { { "stackFrames", frames_json }, { "totalFrames", frames.size() } });
}

void feature_launch::on_scopes(const json& request_seq, const json& args)
{
    json::number_unsigned_t frame_id = args["frameId"].get<json::number_unsigned_t>();

    auto res = ws_mngr_.get_scopes((parser_library::frame_id_t)frame_id);

    json scopes_json = json::array();

    for (size_t i = 0; i < res.size(); ++i)
    {
        auto scope = res.item(i);
        json scope_json = json { { "name", scope.name() },
            { "variablesReference", scope.variable_reference() },
            { "expensive", false },
            { "source", source_to_json(scope.get_source()) } };
        scopes_json.push_back(std::move(scope_json));
    }


    response_->respond(request_seq, "scopes", json { { "scopes", scopes_json } });
}

void feature_launch::on_next(const json& request_seq, const json&)
{
    ws_mngr_.next();

    response_->respond(request_seq, "next", json());
}

void feature_launch::on_step_in(const json& request_seq, const json&)
{
    ws_mngr_.step_in();
    response_->respond(request_seq, "stepIn", json());
}

void feature_launch::on_variables(const json& request_seq, const json& args)
{
    auto var_ref = args["variablesReference"];

    parser_library::variables vars = ws_mngr_.get_variables(var_ref);

    json variables_json = json::array();

    for (size_t i = 0; i < vars.size(); ++i)
    {
        parser_library::variable var = vars.item(i);

        std::string type;
        switch (var.type())
        {
            case parser_library::set_type::A_TYPE:
                type = "A_TYPE";
                break;
            case parser_library::set_type::B_TYPE:
                type = "B_TYPE";
                break;
            case parser_library::set_type::C_TYPE:
                type = "C_TYPE";
                break;
            default:
                break;
        }

        json var_json;
        if (type == "")
            var_json = json {
                { "name", var.name() },
                { "value", var.value() },
                { "variablesReference", var.variable_reference() },
            };
        else
            var_json = json { { "name", var.name() },
                { "value", var.value() },
                { "variablesReference", var.variable_reference() },
                { "type", type } };

        variables_json.push_back(std::move(var_json));
    }

    response_->respond(request_seq, "variables", json { { "variables", variables_json } });
}


void feature_launch::stopped(const char* reason, const char*)
{
    response_->notify(
        "stopped", json { { "reason", reason }, { "threadId", THREAD_ID }, { "allThreadsStopped", true } });
}

void feature_launch::exited(int exit_code) { response_->notify("exited", json { { "exitCode", exit_code } }); }

void feature_launch::terminated() { response_->notify("terminated", json()); }

void feature_launch::on_continue(const json& request_seq, const json&)
{
    ws_mngr_.continue_debug();

    response_->respond(request_seq, "continue", json { { "allThreadsContinued", true } });
}


} // namespace hlasm_plugin::language_server::dap