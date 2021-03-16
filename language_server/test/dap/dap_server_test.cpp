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
#include <filesystem>
#include <stdexcept>
#include <string>
#include <thread>

#include "gmock/gmock.h"

#include "dap/dap_server.h"
#include "platform.h"
#include "workspace_manager.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;

struct send_message_provider_mock : public send_message_provider
{
    void reply(const json& result) override { replies.push_back(result); };

    std::vector<json> replies;
};


TEST(dap_server, dap_server)
{
    std::string file_name = hlasm_plugin::parser_library::platform::absolute_path("to_trace").string();
    file_name[0] = (char)std::tolower((char)file_name[0]);

    std::string file_text = " LR 1,1";
    parser_library::workspace_manager ws_mngr;
    ws_mngr.did_open_file(file_name.c_str(), 0, file_text.c_str(), file_text.size());

    send_message_provider_mock smp;
    dap::server serv(ws_mngr);
    serv.set_send_message_provider(&smp);

    // actual message sent by VS Code
    json initialize_message =
        R"({"command":"initialize","arguments":{"clientID":"vscode","clientName":"Visual Studio Code","adapterID":"hlasm","pathFormat":"path","linesStartAt1":true,"columnsStartAt1":true,"supportsVariableType":true,"supportsVariablePaging":true,"supportsRunInTerminalRequest":true,"locale":"en-us","supportsProgressReporting":true},"type":"request","seq":1})"_json;

    serv.message_received(initialize_message);

    std::vector<json> expected_response_init = {
        R"({"body":{"supportsConfigurationDoneRequest":true},"command":"initialize","request_seq":1,"seq":1,"success":true,"type":"response"})"_json,
        R"({"body":null,"event" : "initialized","seq" : 2,"type" : "event"})"_json
    };

    EXPECT_EQ(smp.replies, expected_response_init);
    smp.replies.clear();


    json disconnect_message =
        R"({"command":"disconnect","arguments":{"restart":false},"type":"request","seq":10})"_json;

    std::vector<json> expected_response_disconnect = {
        R"({"body":null,"command":"disconnect","request_seq":10,"seq":3,"success":true,"type":"response"})"_json
    };

    serv.message_received(disconnect_message);
    EXPECT_EQ(smp.replies, expected_response_disconnect);
    EXPECT_TRUE(serv.is_exit_notification_received());
    EXPECT_TRUE(serv.is_shutdown_request_received());
}

TEST(dap_server, malformed_message)
{
    parser_library::workspace_manager ws_mngr;
    json malf = R"({"commnd":"disconnect"})"_json;
    dap::server serv(ws_mngr);


    serv.message_received(malf);
    // No assertions, the server must not crash.
}
