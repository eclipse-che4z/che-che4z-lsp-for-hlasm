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

#include <memory>

#include "gmock/gmock.h"
#include "json.hpp"

#include "../send_message_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "feature.h"
#include "lsp/feature_text_synchronization.h"
#include "lsp/feature_workspace_folders.h"
#include "lsp/lsp_server.h"
#include "workspace_manager.h"

namespace nlohmann {
// needed in order to have mock methods with json arguments
inline void PrintTo(json const& json, std::ostream* os) { *os << json.dump(); }
} // namespace nlohmann

using namespace hlasm_plugin;
using namespace language_server;

TEST(lsp_server, initialize)
{
    // this is json params actually sent by vscode LSP client
    json j =
        R"({"jsonrpc":"2.0","id":47,"method":"initialize","params":{"processId":5236,"rootPath":null,"rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,"workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off","workspaceFolders":null}})"_json;
    test::ws_mngr_mock ws_mngr;
    send_message_provider_mock smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);



    json server_capab;
    json second;

    json show_message =
        R"({"jsonrpc":"2.0", "method" : "window/showMessage", "params" : {"message":"The capabilities of hlasm language server were sent!", "type" : 3}})"_json;
    json register_message =
        R"({"jsonrpc":"2.0", "id":"register1", "method" : "client/registerCapability", "params" : [{"registrations":[{"id":"configureRegister", "method":"workspace/didChangeConfiguration"}]}]})"_json;
    json config_request_message =
        R"({"id":"config_request_0","jsonrpc":"2.0","method":"workspace/configuration","params":{"items":[{"section":"hlasm"}]}})"_json;

    EXPECT_CALL(smpm, reply(::testing::_)).WillOnce(::testing::SaveArg<0>(&server_capab));
    EXPECT_CALL(smpm, reply(show_message)).Times(::testing::AtMost(1));
    EXPECT_CALL(smpm, reply(register_message)).Times(::testing::AtMost(1));
    EXPECT_CALL(smpm, reply(config_request_message)).Times(::testing::AtMost(1));

    s.message_received(j);

    EXPECT_NE(server_capab.find("jsonrpc"), server_capab.end());
    ASSERT_NE(server_capab.find("id"), server_capab.end());
    EXPECT_EQ(server_capab["id"].get<json::number_unsigned_t>(), 47);
    ASSERT_NE(server_capab.find("result"), server_capab.end());
    EXPECT_NE(server_capab["result"].find("capabilities"), server_capab["result"].end());

    // provide response to the register request
    json register_response = R"({"jsonrpc":"2.0","id":"register1","result":null})"_json;
    s.message_received(register_response);

    json shutdown_request = R"({"jsonrpc":"2.0","id":48,"method":"shutdown","params":null})"_json;
    json shutdown_response = R"({"jsonrpc":"2.0","id":48,"result":null})"_json;
    json exit_notification = R"({"jsonrpc":"2.0","method":"exit","params":null})"_json;
    EXPECT_CALL(smpm, reply(shutdown_response)).Times(1);
    EXPECT_FALSE(s.is_exit_notification_received());
    EXPECT_FALSE(s.is_shutdown_request_received());
    s.message_received(shutdown_request);
    EXPECT_FALSE(s.is_exit_notification_received());
    EXPECT_TRUE(s.is_shutdown_request_received());
    s.message_received(exit_notification);
    EXPECT_TRUE(s.is_exit_notification_received());
    EXPECT_TRUE(s.is_shutdown_request_received());
}

TEST(lsp_server, not_implemented_method)
{
    json j = R"({"jsonrpc":"2.0","id":47,"method":"unknown_method","params":"A parameter"})"_json;
    test::ws_mngr_mock ws_mngr;
    send_message_provider_mock smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);

    s.message_received(j);
    // No result is tested, server should ignore unknown LSP method
}
class request_handler
{
public:
    void handle(json id, json args)
    {
        ++counter;
        received_id = id;
        received_args = args;
    }

    int counter = 0;
    json received_id;
    json received_args;
};

TEST(lsp_server, request_correct)
{
    parser_library::workspace_manager mngr;
    send_message_provider_mock message_provider;
    lsp::server s(mngr);
    s.set_send_message_provider(&message_provider);
    response_provider& rp = s;
    request_handler handler;

    json expected_message =
        R"({"id":"a_request","jsonrpc":"2.0","method":"client_method","params":"a_json_parameter"})"_json;

    EXPECT_CALL(message_provider, reply(expected_message));

    rp.request("a_request",
        "client_method",
        "a_json_parameter",
        std::bind(&request_handler::handle, &handler, std::placeholders::_1, std::placeholders::_2));

    json request_response = R"({"id":"a_request","jsonrpc":"2.0","result":"response_result"})"_json;

    s.message_received(request_response);

    ASSERT_EQ(handler.counter, 1);
    EXPECT_EQ(handler.received_id, "a_request");
    EXPECT_EQ(handler.received_args, "response_result");
}

TEST(lsp_server, request_no_handler)
{
    parser_library::workspace_manager mngr;
    send_message_provider_mock message_provider;
    lsp::server s(mngr);
    s.set_send_message_provider(&message_provider);

    json request_response = R"({"id":"a_request","jsonrpc":"2.0","result":"response_result"})"_json;

    EXPECT_CALL(message_provider, reply(::testing::_)).Times(0);

    s.message_received(request_response);
}

TEST(lsp_server, request_no_id)
{
    parser_library::workspace_manager mngr;
    send_message_provider_mock message_provider;
    lsp::server s(mngr);
    s.set_send_message_provider(&message_provider);

    json request_response = R"({"jsonrpc":"2.0","result":"response_result"})"_json;

    EXPECT_CALL(message_provider, reply(::testing::_)).Times(0);

    s.message_received(request_response);
}



TEST(lsp_server, request_error)
{
    parser_library::workspace_manager mngr;
    send_message_provider_mock message_provider;
    lsp::server s(mngr);
    s.set_send_message_provider(&message_provider);

    json request_response = R"({"id":"a_request","jsonrpc":"2.0","error":{"message":"the_error_message"}})"_json;

    EXPECT_CALL(message_provider, reply(::testing::_)).Times(0);

    s.message_received(request_response);
}

TEST(lsp_server, request_error_no_message)
{
    parser_library::workspace_manager mngr;
    send_message_provider_mock message_provider;
    lsp::server s(mngr);
    s.set_send_message_provider(&message_provider);

    json request_response = R"({"id":"a_request","jsonrpc":"2.0","error":null})"_json;

    EXPECT_CALL(message_provider, reply(::testing::_)).Times(0);

    s.message_received(request_response);
}

TEST(lsp_server_test, wrong_message_received)
{
    test::ws_mngr_mock ws_mngr;
    send_message_provider_mock smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);

    s.message_received(
        R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"user_storage:/user/storage/layout","languageId":"plaintext","version":4,"text":"sad"}}})"_json);
}
