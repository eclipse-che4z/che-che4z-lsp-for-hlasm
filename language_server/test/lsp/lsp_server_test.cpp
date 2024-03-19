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

#include "../send_message_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "feature.h"
#include "lsp/feature_text_synchronization.h"
#include "lsp/feature_workspace_folders.h"
#include "lsp/lsp_server.h"
#include "nlohmann/json.hpp"
#include "workspace_manager.h"

namespace nlohmann {
// needed in order to have mock methods with json arguments
inline void PrintTo(nlohmann::json const& json, std::ostream* os) { *os << json.dump(); }
} // namespace nlohmann

using namespace ::testing;
using namespace hlasm_plugin;
using namespace language_server;

TEST(lsp_server, initialize)
{
    // this is json params actually sent by vscode LSP client
    auto j =
        R"({"jsonrpc":"2.0","id":47,"method":"initialize","params":{"processId":5236,"rootPath":null,"rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,"workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off","workspaceFolders":null}})"_json;
    NiceMock<test::ws_mngr_mock> ws_mngr;
    send_message_provider_mock smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);



    nlohmann::json server_capab;
    nlohmann::json second;

    auto show_message =
        R"({"jsonrpc":"2.0", "method" : "window/showMessage", "params" : {"message":"The capabilities of hlasm language server were sent!", "type" : 3}})"_json;
    auto register_message =
        R"({"jsonrpc":"2.0", "id":0, "method" : "client/registerCapability", "params" : {"registrations":[{"id":"configureRegister", "method":"workspace/didChangeConfiguration"}]}})"_json;
    auto config_request_message =
        R"({"id":1,"jsonrpc":"2.0","method":"workspace/configuration","params":{"items":[{"section":"hlasm"},{}]}})"_json;

    EXPECT_CALL(smpm, reply(_)).WillOnce(SaveArg<0>(&server_capab));
    EXPECT_CALL(smpm, reply(show_message)).Times(AtMost(1));
    EXPECT_CALL(smpm, reply(register_message)).Times(AtMost(1));
    EXPECT_CALL(smpm, reply(config_request_message)).Times(AtMost(1));
    EXPECT_CALL(smpm, reply(Truly([](const nlohmann::json& arg) {
        return arg.count("method") && arg["method"] == "telemetry/event";
    })));

    s.message_received(j);

    EXPECT_NE(server_capab.find("jsonrpc"), server_capab.end());
    ASSERT_NE(server_capab.find("id"), server_capab.end());
    EXPECT_EQ(server_capab["id"].get<nlohmann::json::number_unsigned_t>(), 47);
    ASSERT_NE(server_capab.find("result"), server_capab.end());
    EXPECT_NE(server_capab["result"].find("capabilities"), server_capab["result"].end());

    // provide response to the register request
    auto register_response = R"({"jsonrpc":"2.0","id":0,"result":null})"_json;
    s.message_received(register_response);

    auto shutdown_request = R"({"jsonrpc":"2.0","id":48,"method":"shutdown","params":null})"_json;
    auto shutdown_response = R"({"jsonrpc":"2.0","id":48,"result":null})"_json;
    auto exit_notification = R"({"jsonrpc":"2.0","method":"exit","params":null})"_json;
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
    auto j = R"({"jsonrpc":"2.0","id":47,"method":"unknown_method","params":"A parameter"})"_json;
    NiceMock<test::ws_mngr_mock> ws_mngr;
    send_message_provider_mock smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);

    auto expected_telemetry =
        R"({"jsonrpc":"2.0","method":"telemetry/event","params":{
            "measurements":null,
            "method_name":"server_error/method_not_implemented",
            "properties":{"error_details":"unknown_method"}
           }})"_json;
    auto expected_not_implemented =
        R"({"jsonrpc":"2.0","id":47,"error":{
            "code":-32601,
            "message":"MethodNotFound",
            "data":null
           }})"_json;

    EXPECT_CALL(smpm, reply(expected_not_implemented));
    EXPECT_CALL(smpm, reply(expected_telemetry));

    s.message_received(j);
    // No result is tested, server should ignore unknown LSP method
}
class request_handler
{
public:
    void handle(nlohmann::json args)
    {
        ++counter;
        received_args = args;
    }

    int counter = 0;
    nlohmann::json received_args;
};

TEST(lsp_server, request_correct)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    send_message_provider_mock message_provider;
    lsp::server s(*ws_mngr);
    s.set_send_message_provider(&message_provider);
    response_provider& rp = s;
    request_handler handler;

    auto expected_message = R"({"id":0,"jsonrpc":"2.0","method":"client_method","params":"a_json_parameter"})"_json;

    EXPECT_CALL(message_provider, reply(expected_message));

    rp.request(
        "client_method", "a_json_parameter", [&handler](const nlohmann::json& params) { handler.handle(params); }, {});

    auto request_response = R"({"id":0,"jsonrpc":"2.0","result":"response_result"})"_json;

    s.message_received(request_response);

    ASSERT_EQ(handler.counter, 1);
    EXPECT_EQ(handler.received_args, "response_result");
}

TEST(lsp_server, request_no_handler)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    send_message_provider_mock message_provider;
    lsp::server s(*ws_mngr);
    s.set_send_message_provider(&message_provider);

    auto request_response = R"({"id":"a_request","jsonrpc":"2.0","result":"response_result"})"_json;

    auto expected_telemetry =
        R"({"jsonrpc":"2.0","method":"telemetry/event","params":{
            "measurements":null,
            "method_name":"server_error/lsp_server/response_no_handler",
            "properties":null
           }})"_json;

    // Only telemetry expected
    EXPECT_CALL(message_provider, reply(expected_telemetry));

    s.message_received(request_response);
}

TEST(lsp_server, request_no_id)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    send_message_provider_mock message_provider;
    lsp::server s(*ws_mngr);
    s.set_send_message_provider(&message_provider);

    auto request_response = R"({"jsonrpc":"2.0","result":"response_result"})"_json;

    auto expected_telemetry =
        R"({"jsonrpc":"2.0","method":"telemetry/event","params":{
            "measurements":null,
            "method_name":"server_error/lsp_server/response_no_id",
            "properties":null
           }})"_json;

    // Only telemetry expected
    EXPECT_CALL(message_provider, reply(expected_telemetry));

    s.message_received(request_response);
}

TEST(lsp_server, request_error_unknown)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    send_message_provider_mock message_provider;
    lsp::server s(*ws_mngr);
    s.set_send_message_provider(&message_provider);

    auto request_response = R"({"id":"a_request","jsonrpc":"2.0","error":{"message":"the_error_message"}})"_json;

    auto expected_telemetry =
        R"({"jsonrpc":"2.0","method":"telemetry/event","params":{
            "measurements":null,
            "method_name":"server_error/lsp_server/response_error_returned",
            "properties":{"error_details":"\"the_error_message\""}
           }})"_json;

    // Only telemetry expected
    EXPECT_CALL(message_provider, reply(expected_telemetry));

    s.message_received(request_response);
}

TEST(lsp_server, request_error)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    NiceMock<send_message_provider_mock> message_provider;
    MockFunction<void(int, const char*)> error_handler;
    lsp::server s(*ws_mngr);
    response_provider& rp = s;
    s.set_send_message_provider(&message_provider);

    auto request_response = R"({"id":0,"jsonrpc":"2.0","error":{"code":-123456,"message":"the_error_message"}})"_json;

    // Only telemetry expected
    EXPECT_CALL(error_handler, Call(-123456, StrEq("the_error_message")));

    rp.request("client_method", "args", {}, error_handler.AsStdFunction());

    s.message_received(request_response);
}

TEST(lsp_server, request_error_no_message)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    send_message_provider_mock message_provider;
    lsp::server s(*ws_mngr);
    s.set_send_message_provider(&message_provider);

    auto request_response = R"({"id":"a_request","jsonrpc":"2.0","error":null})"_json;

    auto expected_telemetry =
        R"({"jsonrpc":"2.0","method":"telemetry/event","params":{
            "measurements":null,
            "method_name":"server_error/lsp_server/response_error_returned",
            "properties":{"error_details":"Request with id \"a_request\" returned with unspecified error."}
           }})"_json;

    // Only telemetry expected

    EXPECT_CALL(message_provider, reply(expected_telemetry));

    s.message_received(request_response);
}

TEST(lsp_server_test, non_compliant_uri)
{
    NiceMock<test::ws_mngr_mock> ws_mngr;
    NiceMock<send_message_provider_mock> smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);

    EXPECT_CALL(ws_mngr, did_open_file(StrEq("user_storage:/user/storage/layout"), 4, StrEq("sad"), 3));

    s.message_received(
        R"({"jsonrpc":"2.0","method":"textDocument/didOpen","params":{"textDocument":{"uri":"user_storage:/user/storage/layout","languageId":"plaintext","version":4,"text":"sad"}}})"_json);
}

TEST(lsp_server_test, external_configuration_invalidation)
{
    NiceMock<test::ws_mngr_mock> ws_mngr;
    NiceMock<send_message_provider_mock> smpm;
    lsp::server s(ws_mngr);
    s.set_send_message_provider(&smpm);

    EXPECT_CALL(ws_mngr, invalidate_external_configuration(StrEq("scheme:path")));

    s.message_received(
        R"({"jsonrpc":"2.0","method":"invalidate_external_configuration","params":{"uri":"scheme:path"}})"_json);
}

TEST(lsp_server_test, external_configuration_request)
{
    parser_library::workspace_manager_requests* wmr = nullptr;
    NiceMock<test::ws_mngr_mock> ws_mngr;
    EXPECT_CALL(ws_mngr, set_request_interface(_)).WillOnce(SaveArg<0>(&wmr));

    NiceMock<send_message_provider_mock> smpm;
    lsp::server s(ws_mngr);

    ASSERT_TRUE(wmr);

    s.set_send_message_provider(&smpm);

    struct resp_t
    {
        std::variant<std::string, std::pair<int, std::string>> result;

        void provide(parser_library::sequence<char> c) { result = std::string(c); }
        void error(int err, const char* errmsg) noexcept { result = std::pair<int, std::string>(err, errmsg); }
    };

    auto [c, i] = parser_library::make_workspace_manager_response(std::in_place_type<resp_t>);

    EXPECT_CALL(smpm,
        reply(
            R"({"jsonrpc":"2.0","id":0,"method":"external_configuration_request","params":{"uri":"scheme:path"}})"_json));

    wmr->request_file_configuration(parser_library::sequence<char>(std::string_view("scheme:path")), c);

    s.message_received(R"({"jsonrpc":"2.0","id":0,"result":{"configuration":"GRP1"}})"_json);

    EXPECT_EQ(i->result, (std::variant<std::string, std::pair<int, std::string>>(R"("GRP1")")));
}

TEST(lsp_server_test, external_configuration_request_error)
{
    parser_library::workspace_manager_requests* wmr = nullptr;
    NiceMock<test::ws_mngr_mock> ws_mngr;
    EXPECT_CALL(ws_mngr, set_request_interface(_)).WillOnce(SaveArg<0>(&wmr));

    NiceMock<send_message_provider_mock> smpm;
    lsp::server s(ws_mngr);

    ASSERT_TRUE(wmr);

    s.set_send_message_provider(&smpm);

    struct resp_t
    {
        std::variant<std::string, std::pair<int, std::string>> result;

        void provide(parser_library::sequence<char> c) { result = std::string(c); }
        void error(int err, const char* errmsg) noexcept { result = std::pair<int, std::string>(err, errmsg); }
    };

    auto [c, i] = parser_library::make_workspace_manager_response(std::in_place_type<resp_t>);

    EXPECT_CALL(smpm,
        reply(
            R"({"jsonrpc":"2.0","id":0,"method":"external_configuration_request","params":{"uri":"scheme:path"}})"_json));

    wmr->request_file_configuration(parser_library::sequence<char>(std::string_view("scheme:path")), c);

    s.message_received(R"({"jsonrpc":"2.0","id":0,"error":{"code":123456, "message":"error message"}})"_json);

    EXPECT_EQ(i->result,
        (std::variant<std::string, std::pair<int, std::string>>(std::pair<int, std::string>(123456, "error message"))));
}

TEST(lsp_server_test, toggle_advisory_configuration_diagnostics)
{
    NiceMock<test::ws_mngr_mock> ws_mngr;
    lsp::server s(ws_mngr);

    EXPECT_CALL(ws_mngr, toggle_advisory_configuration_diagnostics());

    s.message_received(
        R"({"jsonrpc":"2.0","method":"toggle_advisory_configuration_diagnostics","params":[null]})"_json);
}
