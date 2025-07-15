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
#include "nlohmann/json.hpp"
#include "utils/path.h"
#include "utils/platform.h"
#include "workspace_manager.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;

struct send_message_provider_mock : public send_message_provider
{
    void reply(nlohmann::json&& result) override { replies.push_back(std::move(result)); };

    std::vector<nlohmann::json> replies;
};


TEST(dap_server, dap_server)
{
    std::string file_name = utils::path::absolute("to_trace").string();
    file_name[0] = (char)std::tolower((unsigned char)file_name[0]);

    std::string file_text = " LR 1,1";
    auto ws_mngr = parser_library::create_workspace_manager();
    ws_mngr->did_open_file(file_name, 0, file_text);
    ws_mngr->idle_handler();

    send_message_provider_mock smp;
    dap::server serv(ws_mngr->get_debugger_configuration_provider());
    serv.set_send_message_provider(&smp);

    // actual message sent by VS Code
    auto initialize_message =
        R"({"command":"initialize","arguments":{"clientID":"vscode","clientName":"Visual Studio Code","adapterID":"hlasm","pathFormat":"path","linesStartAt1":true,"columnsStartAt1":true,"supportsVariableType":true,"supportsVariablePaging":true,"supportsRunInTerminalRequest":true,"locale":"en-us","supportsProgressReporting":true},"type":"request","seq":1})"_json;

    serv.message_received(initialize_message);

    std::vector expected_response_init = {
        R"({"body":{"supportsConfigurationDoneRequest":true,"supportsEvaluateForHovers":true,"supportsFunctionBreakpoints":true},"command":"initialize","request_seq":1,"seq":1,"success":true,"type":"response"})"_json,
        R"({"body":null,"event" : "initialized","seq" : 2,"type" : "event"})"_json
    };

    EXPECT_EQ(smp.replies, expected_response_init);
    smp.replies.clear();


    auto disconnect_message =
        R"({"command":"disconnect","arguments":{"restart":false},"type":"request","seq":10})"_json;

    std::vector expected_response_disconnect = {
        R"({"body":null,"command":"disconnect","request_seq":10,"seq":3,"success":true,"type":"response"})"_json
    };

    serv.message_received(disconnect_message);
    EXPECT_EQ(smp.replies, expected_response_disconnect);
    EXPECT_TRUE(serv.is_exit_notification_received());
    EXPECT_TRUE(serv.is_shutdown_request_received());
}

TEST(dap_server, malformed_message)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    auto malf = R"({"commnd":"disconnect"})"_json;
    dap::server serv(ws_mngr->get_debugger_configuration_provider());


    serv.message_received(malf);
    // No assertions, the server must not crash.
}
