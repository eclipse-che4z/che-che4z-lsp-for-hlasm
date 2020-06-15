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

#include "dap/feature_launch.h"
#include "workspace_manager.h"


using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::language_server::dap;
using namespace std::chrono_literals;

struct response_mock
{
    json id;
    std::string req_method;
    json args;

    friend bool operator==(const response_mock& lhs, const response_mock& rhs)
    {
        return lhs.id == rhs.id && lhs.req_method == rhs.req_method && lhs.args == rhs.args;
    }
};

struct notif_mock
{
    std::string req_method;
    json args;

    friend bool operator==(const notif_mock& lhs, const notif_mock& rhs)
    {
        return lhs.req_method == rhs.req_method && lhs.args == rhs.args;
    }
};



struct response_provider_mock : public response_provider
{
    void respond(const json& id, const std::string& requested_method, const json& args) override
    {
        responses.push_back({ id, requested_method, args });
    };
    void notify(const std::string& method, const json& args) override
    {
        notifs.push_back({ method, args });
        if (method == "stopped")
            stopped = true;
        if (method == "exited")
            exited = true;
    };
    void respond_error(const json&, const std::string&, int, const std::string&, const json&) override {};

    std::vector<response_mock> responses;
    std::vector<notif_mock> notifs;
    std::atomic<bool> stopped = false;
    std::atomic<bool> exited = false;

    void wait_for_stopped()
    {
        size_t i = 0;
        while (!stopped)
        {
            if (i > 50)
                throw std::runtime_error("Wait for stopped timeout.");
            ++i;

            std::this_thread::sleep_for(100ms);
        }
        stopped = false;
    }

    void wait_for_exited()
    {
        size_t i = 0;
        while (!exited)
        {
            if (i > 50)
                throw std::runtime_error("Wait for exited timeout.");
            ++i;

            std::this_thread::sleep_for(100ms);
        }
        exited = false;
    }


    void reset()
    {
        responses.clear();
        notifs.clear();
    }
};

struct feature_launch_test : public testing::Test
{
    feature_launch_test()
        : feat(ws_mngr, resp_provider)
    {
        feat.register_methods(methods);
        feat.initialize_feature(R"({"linesStartAt1":false, "columnsStartAt1":false, "pathFormat":"path"})"_json);

        file_name = std::filesystem::absolute("to_trace").string();
        file_name[0] = std::tolower((char)file_name[0]);
    }

    void check_simple_stack_trace(json id, size_t expected_line)
    {
        methods["stackTrace"]("1"_json, json());
        ASSERT_EQ(resp_provider.responses.size(), 1U);
        const response_mock& r = resp_provider.responses[0];
        EXPECT_EQ(r.id, "1"_json);
        EXPECT_EQ(r.req_method, "stackTrace");
        ASSERT_EQ(r.args["totalFrames"], 1U);
        ASSERT_EQ(r.args["stackFrames"].size(), 1U);
        const json& f = r.args["stackFrames"][0];
        EXPECT_EQ(f["name"], "OPENCODE");
        json expected_source { { "path", file_name } };
        EXPECT_EQ(f["source"], expected_source);
        EXPECT_EQ(f["line"], expected_line);
        EXPECT_EQ(f["endLine"], expected_line);
        EXPECT_EQ(f["column"], 0);
        EXPECT_EQ(f["endColumn"], 0);
        resp_provider.reset();
    }

    void wait_for_stopped()
    {
        resp_provider.wait_for_stopped();
        EXPECT_EQ(resp_provider.notifs.size(), 1U);
    }

    std::map<std::string, method> methods;
    response_provider_mock resp_provider;
    parser_library::workspace_manager ws_mngr;
    feature_launch feat;
    std::string file_name;
};

std::string file_stop_on_entry = "  LR 1,1";

TEST_F(feature_launch_test, stop_on_entry)
{
    ws_mngr.did_open_file(file_name.c_str(), 0, file_stop_on_entry.c_str(), file_stop_on_entry.size());


    methods["launch"]("0"_json, json { { "program", file_name }, { "stopOnEntry", true } });
    std::vector<response_mock> expected_resp = { { "0"_json, "launch", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace("1"_json, 0);

    ws_mngr.disconnect();
}


std::string file_step = R"(  LR 1,1
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
    ws_mngr.did_open_file(file_name.c_str(), 0, file_step.c_str(), file_step.size());

    methods["launch"]("0"_json, json { { "program", file_name }, { "stopOnEntry", true } });
    std::vector<response_mock> expected_resp = { { "0"_json, "launch", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();


    check_simple_stack_trace("2"_json, 0);

    methods["stepIn"]("3"_json, json());
    expected_resp = { { "3"_json, "stepIn", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace("4"_json, 1);

    methods["next"]("5"_json, json());
    expected_resp = { { "5"_json, "next", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace("6"_json, 6);

    methods["next"]("7"_json, json());
    expected_resp = { { "7"_json, "next", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace("8"_json, 7);

    methods["stepIn"]("8"_json, json());
    expected_resp = { { "8"_json, "stepIn", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();

    methods["stackTrace"]("9"_json, json());
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    const response_mock& r = resp_provider.responses[0];
    EXPECT_EQ(r.id, "9"_json);
    EXPECT_EQ(r.req_method, "stackTrace");
    ASSERT_EQ(r.args["totalFrames"], 2U);
    ASSERT_EQ(r.args["stackFrames"].size(), 2U);
    const json& fs = r.args["stackFrames"];
    size_t expected_lines[2] = { 3, 7 };
    std::string expected_names[2] = { "MACRO", "OPENCODE" };
    for (size_t i = 0; i < 2; ++i)
    {
        const json& f = fs[i];
        EXPECT_EQ(f["name"], expected_names[i]);
        json expected_source { { "path", file_name } };
        EXPECT_EQ(f["source"], expected_source);
        EXPECT_EQ(f["line"], expected_lines[i]);
        EXPECT_EQ(f["endLine"], expected_lines[i]);
        EXPECT_EQ(f["column"], 0);
        EXPECT_EQ(f["endColumn"], 0);
    }
    resp_provider.reset();

    methods["continue"]("47"_json, json());
    resp_provider.wait_for_exited();
}

std::string file_breakpoint = R"(  LR 1,1
  LR 1,1  First breakpoint comes on this line

  LR 1,1  Second breakpoint on this line
)";

TEST_F(feature_launch_test, breakpoint)
{
    ws_mngr.did_open_file(file_name.c_str(), 0, file_breakpoint.c_str(), file_breakpoint.size());


    json bp_args { { "source", { { "path", file_name } } }, { "breakpoints", R"([{"line":1}, {"line":3}])"_json } };
    methods["setBreakpoints"]("47"_json, bp_args);
    std::vector<response_mock> expected_resp_bp = { { "47"_json, "setBreakpoints", R"(
        { "breakpoints":[
            {"verified":true},
            {"verified":true}
        ]})"_json } };
    EXPECT_EQ(resp_provider.responses, expected_resp_bp);
    resp_provider.reset();

    methods["launch"]("0"_json, json { { "program", file_name }, { "stopOnEntry", false } });
    std::vector<response_mock> expected_resp = { { "0"_json, "launch", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();


    check_simple_stack_trace("2"_json, 1);

    methods["continue"]("3"_json, json());
    std::vector<response_mock> expected_resp_cont = {
        { "3"_json, "continue", json { { "allThreadsContinued", true } } }
    };
    EXPECT_EQ(resp_provider.responses, expected_resp_cont);
    wait_for_stopped();
    resp_provider.reset();

    check_simple_stack_trace("4"_json, 3);

    ws_mngr.disconnect();
}

std::string file_variables = R"(&VARA SETA 4
&VARB SETB 1
&VARC SETC 'STH'
      LR 1,1)";

TEST_F(feature_launch_test, variables)
{
    ws_mngr.did_open_file(file_name.c_str(), 0, file_variables.c_str(), file_variables.size());

    methods["launch"]("0"_json, json { { "program", file_name }, { "stopOnEntry", true } });
    std::vector<response_mock> expected_resp = { { "0"_json, "launch", json() } };
    EXPECT_EQ(resp_provider.responses, expected_resp);
    wait_for_stopped();
    resp_provider.reset();


    check_simple_stack_trace("2"_json, 0);

    methods["stepIn"]("3"_json, json());
    resp_provider.wait_for_stopped();
    methods["stepIn"]("3"_json, json());
    resp_provider.wait_for_stopped();
    methods["stepIn"]("3"_json, json());
    resp_provider.wait_for_stopped();
    resp_provider.reset();

    methods["stackTrace"]("1"_json, json());
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    response_mock r = resp_provider.responses[0];
    EXPECT_EQ(r.id, "1"_json);
    EXPECT_EQ(r.req_method, "stackTrace");
    ASSERT_EQ(r.args["totalFrames"], 1U);
    ASSERT_EQ(r.args["stackFrames"].size(), 1U);
    json f = r.args["stackFrames"][0];
    EXPECT_EQ(f["name"], "OPENCODE");
    json expected_source { { "path", file_name } };
    EXPECT_EQ(f["source"], expected_source);
    EXPECT_EQ(f["line"], 3U);
    EXPECT_EQ(f["endLine"], 3U);
    EXPECT_EQ(f["column"], 0);
    EXPECT_EQ(f["endColumn"], 0);
    json frame_id = f["id"];
    resp_provider.reset();

    methods["scopes"]("5"_json, json { { "frameId", frame_id } });
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    r = resp_provider.responses[0];
    EXPECT_EQ(r.id, "5"_json);
    EXPECT_EQ(r.req_method, "scopes");
    json scopes = r.args["scopes"];
    json locals_ref;
    for (const json & scope : scopes)
    {
        EXPECT_FALSE(scope.find("name") == scope.end());
        EXPECT_FALSE(scope.find("variablesReference") == scope.end());
        EXPECT_EQ(scope["expensive"], json(false));
        EXPECT_EQ(scope["source"], expected_source);
        if (scope["name"] == "Locals")
            locals_ref = scope["variablesReference"];
    }
    resp_provider.reset();

    methods["variables"]("8"_json, json { { "variablesReference", locals_ref } });
    ASSERT_EQ(resp_provider.responses.size(), 1U);
    r = resp_provider.responses[0];
    EXPECT_EQ(r.id, "8"_json);
    EXPECT_EQ(r.req_method, "variables");
    const json & variables = r.args["variables"];
    size_t var_count = 0;
    for (const json & var : variables)
    {
        EXPECT_FALSE(var.find("name") == var.end());
        EXPECT_FALSE(var.find("value") == var.end());
        EXPECT_FALSE(var.find("variablesReference") == var.end());
        if (var["name"] == "VARA")
        {
            EXPECT_EQ(var["value"], json("4"));
            EXPECT_EQ(var["type"], json("A_TYPE"));
            ++var_count;
        }
        else if (var["name"] == "VARB")
        {
            EXPECT_EQ(var["value"], json("TRUE"));
            EXPECT_EQ(var["type"], json("B_TYPE"));
            ++var_count;
        }
        else if (var["name"] == "VARC")
        {
            EXPECT_EQ(var["value"], json("STH"));
            EXPECT_EQ(var["type"], json("C_TYPE"));
            ++var_count;
        }
    }
    //test, that all variable symbols were reported
    EXPECT_EQ(var_count, 3);

    ws_mngr.disconnect();
}