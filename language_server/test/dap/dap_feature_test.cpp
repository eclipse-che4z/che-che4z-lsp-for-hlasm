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

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

#include "gmock/gmock.h"

#include "dap/dap_server.h"
#include "feature.h"
#include "nlohmann/json.hpp"
#include "utils/path.h"
#include "utils/path_conversions.h"
#include "utils/platform.h"
#include "workspace_manager.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::language_server::dap;
using namespace std::chrono_literals;

struct response_mock
{
    request_id id;
    std::string req_method;
    nlohmann::json args;

    friend bool operator==(const response_mock& lhs, const response_mock& rhs)
    {
        return lhs.id == rhs.id && lhs.req_method == rhs.req_method && lhs.args == rhs.args;
    }
};

struct notif_mock
{
    std::string req_method;
    nlohmann::json args;

    friend bool operator==(const notif_mock& lhs, const notif_mock& rhs)
    {
        return lhs.req_method == rhs.req_method && lhs.args == rhs.args;
    }
};



struct response_provider_mock : public response_provider
{
    void request(const std::string&,
        const nlohmann::json&,
        std::function<void(const nlohmann::json& params)>,
        std::function<void(int, const char*)>) override
    {}
    void respond(const request_id& id, const std::string& requested_method, const nlohmann::json& args) override
    {
        responses.push_back({ id, requested_method, args });
    }
    void notify(const std::string& method, const nlohmann::json& args) override
    {
        notifs.push_back({ method, args });
        if (method == "stopped")
            stopped = true;
        if (method == "exited")
            exited = true;
    }
    void respond_error(const request_id&, const std::string&, int, const std::string&, const nlohmann::json&) override
    {}

    void register_cancellable_request(const request_id&, request_invalidator) override {}

    std::vector<response_mock> responses;
    std::vector<notif_mock> notifs;
    bool stopped = false;
    bool exited = false;


    void reset()
    {
        responses.clear();
        notifs.clear();
    }
};

struct feature_launch_test : public testing::Test
{
    feature_launch_test()
        : feature(ws_mngr->get_debugger_configuration_provider(), resp_provider, nullptr)
    {
        feature.on_initialize(
            request_id(0), R"({"linesStartAt1":false, "columnsStartAt1":false, "pathFormat":"path"})"_json);
        resp_provider.reset();

        if (utils::platform::is_web())
        {
            file_path = "file://to_trace";
        }
        else
        {
            file_path = hlasm_plugin::utils::path::absolute("to_trace").string();
            file_path[0] = (char)std::tolower((unsigned char)file_path[0]);
        }
    }

    void check_simple_stack_trace(nlohmann::json, size_t expected_line)
    {
        feature.on_stack_trace(request_id(1), nlohmann::json());
        ASSERT_EQ(resp_provider.responses.size(), 1U);
        const response_mock& r = resp_provider.responses[0];
        EXPECT_EQ(r.id, request_id(1));
        EXPECT_EQ(r.req_method, "stackTrace");
        ASSERT_EQ(r.args["totalFrames"], 1U);
        ASSERT_EQ(r.args["stackFrames"].size(), 1U);
        const nlohmann::json& f = r.args["stackFrames"][0];
        EXPECT_EQ(f["name"], "OPENCODE");
        nlohmann::json expected_source { { "path", file_path } };
        EXPECT_EQ(f["source"], expected_source);
        EXPECT_EQ(f["line"], expected_line);
        EXPECT_EQ(f["endLine"], expected_line);
        EXPECT_EQ(f["column"], 0);
        EXPECT_EQ(f["endColumn"], 0);
        resp_provider.reset();
    }

    void wait_for_stopped()
    {
        for (int i = 0; !resp_provider.stopped; ++i)
        {
            if (i >= 1000000)
                throw std::runtime_error("Wait for stopped timeout.");

            feature.idle_handler(nullptr);
        }
        EXPECT_EQ(resp_provider.notifs.size(), 1U);
        resp_provider.stopped = false;
    }

    void wait_for_exited()
    {
        for (int i = 0; !resp_provider.exited; ++i)
        {
            if (i >= 1000000)
                throw std::runtime_error("Wait for exited timeout.");

            feature.idle_handler(nullptr);
        }
        resp_provider.exited = false;
    }

    std::map<std::string, method> methods;
    response_provider_mock resp_provider;
    std::unique_ptr<parser_library::workspace_manager> ws_mngr = parser_library::create_workspace_manager();
    dap::dap_feature feature;
    std::string file_path;
};

const std::string file_stop_on_entry = "  LR 1,1";

TEST_F(feature_launch_test, stop_on_entry)
{
    ws_mngr->did_open_file(
        utils::path::path_to_uri(file_path).c_str(), 0, file_stop_on_entry.c_str(), file_stop_on_entry.size());
    ws_mngr->idle_handler();

    feature.on_launch(request_id(0), nlohmann::json { { "program", file_path }, { "stopOnEntry", true } });
    ws_mngr->idle_handler();
    feature.idle_handler(nullptr);
    std::vector<response_mock> expected_resp = { { request_id(0), "launch", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(1), 0);

    feature.on_disconnect(request_id(2), {});
}

const std::string file_step = R"(  LR 1,1
  MACRO
  MAC
  LR 1,1
  MEND

  MAC
  MAC

  LR 1,1  Second breakpoint on this line
)";

TEST_F(feature_launch_test, step)
{
    ws_mngr->did_open_file(utils::path::path_to_uri(file_path).c_str(), 0, file_step.c_str(), file_step.size());
    ws_mngr->idle_handler();

    feature.on_launch(request_id(0), nlohmann::json { { "program", file_path }, { "stopOnEntry", true } });
    ws_mngr->idle_handler();
    feature.idle_handler(nullptr);
    std::vector<response_mock> expected_resp = { { request_id(0), "launch", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(2), 0);

    feature.on_step_in(request_id(3), nlohmann::json());
    expected_resp = { { request_id(3), "stepIn", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(4), 1);

    feature.on_next(request_id(5), nlohmann::json());
    expected_resp = { { request_id(5), "next", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(6), 6);

    feature.on_next(request_id(7), nlohmann::json());
    expected_resp = { { request_id(7), "next", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(8), 7);

    feature.on_step_in(request_id(8), nlohmann::json());
    expected_resp = { { request_id(8), "stepIn", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    feature.on_stack_trace(request_id(9), nlohmann::json());
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    const response_mock& r = resp_provider.responses[0];
    EXPECT_EQ(r.id, request_id(9));
    EXPECT_EQ(r.req_method, "stackTrace");
    ASSERT_EQ(r.args["totalFrames"], 2U);
    ASSERT_EQ(r.args["stackFrames"].size(), 2U);
    const nlohmann::json& fs = r.args["stackFrames"];
    size_t expected_lines[2] = { 3, 7 };
    std::string expected_names[2] = { "MACRO", "OPENCODE" };
    for (size_t i = 0; i < 2; ++i)
    {
        const nlohmann::json& f = fs[i];
        EXPECT_EQ(f["name"], expected_names[i]);
        nlohmann::json expected_source { { "path", file_path } };
        EXPECT_EQ(f["source"], expected_source);
        EXPECT_EQ(f["line"], expected_lines[i]);
        EXPECT_EQ(f["endLine"], expected_lines[i]);
        EXPECT_EQ(f["column"], 0);
        EXPECT_EQ(f["endColumn"], 0);
    }
    resp_provider.reset();

    feature.on_step_out(request_id(10), nlohmann::json());
    expected_resp = { { request_id(10), "stepOut", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    feature.on_stack_trace(request_id(11), nlohmann::json());
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    const response_mock& r2 = resp_provider.responses[0];
    EXPECT_EQ(r2.id, request_id(11));
    EXPECT_EQ(r2.req_method, "stackTrace");
    ASSERT_EQ(r2.args["totalFrames"], 1U);
    ASSERT_EQ(r2.args["stackFrames"].size(), 1U);

    feature.on_continue(request_id(47), nlohmann::json());
    wait_for_exited();
    feature.on_disconnect(request_id(48), {});
}

const std::string file_breakpoint = R"(  LR 1,1
  LR 1,1  First breakpoint comes on this line

  LR 1,1  Second breakpoint on this line
)";

TEST_F(feature_launch_test, breakpoint)
{
    ws_mngr->did_open_file(
        utils::path::path_to_uri(file_path).c_str(), 0, file_breakpoint.c_str(), file_breakpoint.size());
    ws_mngr->idle_handler();

    nlohmann::json bp_args { { "source", { { "path", file_path } } },
        { "breakpoints", R"([{"line":1}, {"line":3}])"_json } };
    feature.on_set_breakpoints(request_id(47), bp_args);
    std::vector<response_mock> expected_resp_bp = { { request_id(47), "setBreakpoints", R"(
        { "breakpoints":[
            {"verified":true},
            {"verified":true}
        ]})"_json } };
    EXPECT_EQ(resp_provider.responses, expected_resp_bp);
    resp_provider.reset();

    feature.on_launch(request_id(0), nlohmann::json { { "program", file_path }, { "stopOnEntry", false } });
    ws_mngr->idle_handler();
    feature.idle_handler(nullptr);
    std::vector<response_mock> expected_resp = { { request_id(0), "launch", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(2), 1);

    feature.on_continue(request_id(3), nlohmann::json());
    std::vector<response_mock> expected_resp_cont = {
        { request_id(3), "continue", nlohmann::json { { "allThreadsContinued", true } } }
    };
    EXPECT_EQ(resp_provider.responses, expected_resp_cont);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(4), 3);

    feature.on_disconnect(request_id(5), {});
}

const std::string file_variables = R"(&VARA SETA 4
&VARB SETB 1
&VARC SETC 'STH'
      LR 1,1)";

TEST_F(feature_launch_test, variables)
{
    ws_mngr->did_open_file(
        utils::path::path_to_uri(file_path).c_str(), 0, file_variables.c_str(), file_variables.size());
    ws_mngr->idle_handler();

    feature.on_launch(request_id(0), nlohmann::json { { "program", file_path }, { "stopOnEntry", true } });
    ws_mngr->idle_handler();
    feature.idle_handler(nullptr);
    std::vector<response_mock> expected_resp = { { request_id(0), "launch", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace(request_id(2), 0);

    feature.on_step_in(request_id(3), nlohmann::json());
    wait_for_stopped();
    resp_provider.reset();
    feature.on_step_in(request_id(3), nlohmann::json());
    wait_for_stopped();
    resp_provider.reset();
    feature.on_step_in(request_id(3), nlohmann::json());
    wait_for_stopped();
    resp_provider.reset();

    feature.on_stack_trace(request_id(1), nlohmann::json());
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    response_mock r = resp_provider.responses[0];
    EXPECT_EQ(r.id, request_id(1));
    EXPECT_EQ(r.req_method, "stackTrace");
    ASSERT_EQ(r.args["totalFrames"], 1U);
    ASSERT_EQ(r.args["stackFrames"].size(), 1U);
    nlohmann::json f = r.args["stackFrames"][0];
    EXPECT_EQ(f["name"], "OPENCODE");
    nlohmann::json expected_source { { "path", file_path } };
    EXPECT_EQ(f["source"], expected_source);
    EXPECT_EQ(f["line"], 3U);
    EXPECT_EQ(f["endLine"], 3U);
    EXPECT_EQ(f["column"], 0);
    EXPECT_EQ(f["endColumn"], 0);
    nlohmann::json frame_id = f["id"];
    resp_provider.reset();

    feature.on_scopes(request_id(5), nlohmann::json { { "frameId", frame_id } });
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    r = resp_provider.responses[0];
    EXPECT_EQ(r.id, request_id(5));
    EXPECT_EQ(r.req_method, "scopes");
    nlohmann::json scopes = r.args["scopes"];
    nlohmann::json locals_ref;
    for (const nlohmann::json& scope : scopes)
    {
        EXPECT_FALSE(scope.find("name") == scope.end());
        EXPECT_FALSE(scope.find("variablesReference") == scope.end());
        EXPECT_EQ(scope["expensive"], nlohmann::json(false));
        EXPECT_EQ(scope["source"], expected_source);
        if (scope["name"] == "Locals")
            locals_ref = scope["variablesReference"];
    }
    resp_provider.reset();

    feature.on_variables(request_id(8), nlohmann::json { { "variablesReference", locals_ref } });
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    r = resp_provider.responses[0];
    EXPECT_EQ(r.id, request_id(8));
    EXPECT_EQ(r.req_method, "variables");
    const nlohmann::json& variables = r.args["variables"];
    size_t var_count = 0;
    for (const nlohmann::json& var : variables)
    {
        EXPECT_FALSE(var.find("name") == var.end());
        EXPECT_FALSE(var.find("value") == var.end());
        EXPECT_FALSE(var.find("variablesReference") == var.end());
        if (var["name"] == "&VARA")
        {
            EXPECT_EQ(var["value"], nlohmann::json("4"));
            EXPECT_EQ(var["type"], nlohmann::json("A_TYPE"));
            ++var_count;
        }
        else if (var["name"] == "&VARB")
        {
            EXPECT_EQ(var["value"], nlohmann::json("TRUE"));
            EXPECT_EQ(var["type"], nlohmann::json("B_TYPE"));
            ++var_count;
        }
        else if (var["name"] == "&VARC")
        {
            EXPECT_EQ(var["value"], nlohmann::json("STH"));
            EXPECT_EQ(var["type"], nlohmann::json("C_TYPE"));
            ++var_count;
        }
    }
    // test, that all variable symbols were reported
    EXPECT_EQ(var_count, 3);

    feature.on_disconnect(request_id(9), {});
}

namespace {
const std::string pause_file = ".A AGO .A";
}

TEST_F(feature_launch_test, pause)
{
    ws_mngr->did_open_file(utils::path::path_to_uri(file_path).c_str(), 0, pause_file.c_str(), pause_file.size());
    ws_mngr->idle_handler();

    feature.on_launch(request_id(0), nlohmann::json { { "program", file_path }, { "stopOnEntry", false } });
    ws_mngr->idle_handler();

    std::atomic<unsigned char> yield = 1;

    feature.idle_handler(&yield);

    feature.on_pause(request_id(1), {});

    wait_for_stopped();
    std::vector<response_mock> expected_resp = { { request_id(0), "launch", nlohmann::json() },
        { request_id(1), "pause", nlohmann::json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    resp_provider.reset();

    feature.on_disconnect(request_id(2), {});
}
