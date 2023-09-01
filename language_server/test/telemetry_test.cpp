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

#include <sstream>
#include <thread>

#include "gmock/gmock.h"

#include "dap/dap_server.h"
#include "lsp/lsp_server.h"
#include "nlohmann/json.hpp"
#include "send_message_provider_mock.h"
#include "stream_helper.h"
#include "telemetry_broker.h"
#include "workspace_manager.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;

using ::testing::SaveArg;
using ::testing::Truly;

namespace {
nlohmann::json open_file_message =
    R"({"jsonrpc":"2.0","id":47,"method":"textDocument/didOpen","params":{"textDocument":{"uri":"file:///test_file","languageId":"hlasm","version":4,"text":" LR 16,1"}}})"_json;

auto get_method_matcher(std::string method)
{
    return [method = std::move(method)](const nlohmann::json& arg) {
        return arg.count("method") > 0 && arg["method"].get<std::string>() == method;
    };
}

auto get_telemetry_method_matcher(std::string method)
{
    return [method = std::move(method)](const nlohmann::json& arg) {
        return arg.count("method") > 0 && arg["method"].get<std::string>() == "telemetry/event"
            && arg["params"]["method_name"] == method;
    };
}

} // namespace
TEST(telemetry, lsp_server_did_open)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    lsp::server lsp_server(*ws_mngr);
    send_message_provider_mock lsp_smpm;
    lsp_server.set_send_message_provider(&lsp_smpm);

    nlohmann::json diags_reply;
    nlohmann::json telemetry_reply;

    EXPECT_CALL(lsp_smpm, reply(Truly(get_method_matcher("external_configuration_request"))))
        .WillRepeatedly([&lsp_server](const nlohmann ::json& j) {
            lsp_server.message_received({
                { "id", j.at("id") },
                { "jsonrpc", "2.0" },
                {
                    "error",
                    {
                        { "code", 0 },
                        { "message", "Not found" },
                    },
                },
            });
        });

    EXPECT_CALL(lsp_smpm, reply(Truly(get_method_matcher("textDocument/publishDiagnostics"))))
        .WillOnce(SaveArg<0>(&diags_reply));

    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("parsing")))).WillOnce(SaveArg<0>(&telemetry_reply));
    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("textDocument/didOpen"))));

    lsp_server.message_received(open_file_message);
    ws_mngr->idle_handler();

    EXPECT_EQ(diags_reply["params"]["diagnostics"].size(), 1);

    nlohmann::json& metrics = telemetry_reply["params"]["measurements"];

    EXPECT_GT(metrics["duration"], 0U);
    EXPECT_EQ(metrics["error_count"], 1);

    nlohmann::json& ws_info = telemetry_reply["params"]["properties"];

    EXPECT_EQ(ws_info["files_processed"], 1);
    EXPECT_EQ(ws_info["diagnostics_suppressed"], false);
}

TEST(telemetry, telemetry_broker)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    lsp::server lsp_server(*ws_mngr);
    send_message_provider_mock lsp_smpm;
    lsp_server.set_send_message_provider(&lsp_smpm);

    EXPECT_CALL(lsp_smpm, reply(Truly(get_method_matcher("external_configuration_request"))))
        .WillRepeatedly([&lsp_server](const nlohmann ::json& j) {
            lsp_server.message_received({
                { "id", j.at("id") },
                { "jsonrpc", "2.0" },
                {
                    "error",
                    {
                        { "code", 0 },
                        { "message", "Not found" },
                    },
                },
            });
        });

    EXPECT_CALL(lsp_smpm, reply(Truly(get_method_matcher("textDocument/publishDiagnostics"))));


    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("parsing"))));
    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("textDocument/didOpen"))));

    EXPECT_CALL(
        lsp_smpm, reply(Truly([](auto& arg) { return arg.count("id") > 0 && arg["id"] == 48; }))); // response to
    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("textDocument/hover"))));
    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("launch"))));
    EXPECT_CALL(lsp_smpm, reply(Truly(get_telemetry_method_matcher("disconnect"))));


    telemetry_broker broker;
    broker.set_telemetry_sink(&lsp_server);

    lsp_server.message_received(open_file_message);
    ws_mngr->idle_handler();

    //"textDocument/hover",R"#({"textDocument":{"uri":"file:///c%3A/test/stability.hlasm"},"position":{"line":0,"character":7}})#"_json),

    std::thread lsp_thread([&lsp_server, &ws_mngr]() {
        lsp_server.message_received(
            R"({"jsonrpc":"2.0","id":48,"method":"textDocument/hover","params":{"textDocument":{"uri":"file:///test_file"},"position":{"line":0,"character":2} }})"_json);
        ws_mngr->idle_handler();
    });
    lsp_thread.join();


    std::thread dap_thread([&]() {
        dap::server dap_server(ws_mngr->get_debugger_configuration_provider(), &broker);
        ::testing::NiceMock<send_message_provider_mock> dap_smpm;
        dap_server.set_send_message_provider(&dap_smpm);

        // actual message sent by VS Code
        auto initialize_message =
            R"({"command":"initialize","arguments":{"pathFormat":"uri","linesStartAt1":true,"columnsStartAt1":true},"type":"request","seq":1})"_json;

        dap_server.message_received(initialize_message);

        auto launch_message =
            R"({"command":"launch","arguments":{"program":"file:///test_file","stopOnEntry":true,"restart":false},"type":"request","seq":10})"_json;

        dap_server.message_received(launch_message);
        ws_mngr->idle_handler();
        dap_server.idle_handler(nullptr);

        auto disconnect_message =
            R"({"command":"disconnect","arguments":{"restart":false},"type":"request","seq":10})"_json;

        dap_server.message_received(disconnect_message);
    });
    dap_thread.join();
}
