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


#include <string>

#include "../response_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "../ws_mngr_req_mock.h"
#include "lib_config.h"
#include "lsp/feature_workspace_folders.h"
#include "nlohmann/json.hpp"
#include "utils/platform.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using hlasm_plugin::utils::platform::is_windows;

const std::string ws1_uri = is_windows() ? "file:///c%3A/path/to/W%20S/OneDrive" : "file:///path/to/W%20S/OneDrive";
const std::string ws2_uri = is_windows() ? "file:///c%3A/path/to/W%20S/TwoDrive" : "file:///path/to/W%20S/TwoDrive";
const std::string ws3_uri = is_windows() ? "file:///c%3A/path/to/W%20S/ThreeDrive" : "file:///path/to/W%20S/ThreeDrive";
const std::string ws4_uri = is_windows() ? "file:///c%3A/path/to/W%20S/FourDrive" : "file:///path/to/W%20S/FourDrive";

const std::string ws1_path_json_string = is_windows() ? R"(c:\\path\\to\\W S\\OneDrive)" : R"(/path/to/W S/OneDrive)";

TEST(workspace_folders, did_change_workspace_folders)
{
    test::ws_mngr_mock ws_mngr;
    response_provider_mock rpm;

    lsp::feature_workspace_folders f(ws_mngr, rpm);

    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("OneDrive"), ::testing::StrEq(ws1_uri)));

    auto params1 =
        nlohmann::json::parse(R"({"event":{"added":[{"uri":")" + ws1_uri + R"(","name":"OneDrive"}],"removed":[]}})");
    notifs["workspace/didChangeWorkspaceFolders"].as_notification_handler()(params1);

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("TwoDrive"), ::testing::StrEq(ws2_uri)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("ThreeDrive"), ::testing::StrEq(ws3_uri)));
    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws1_uri)));

    auto params2 = nlohmann::json::parse(R"({"event":{"added":[{"uri":")" + ws2_uri + R"(","name":"TwoDrive"},{"uri":")"
        + ws3_uri + R"(","name":"ThreeDrive"}],"removed":[{"uri":")" + ws1_uri + R"(","name":"OneDrive"}]}})");
    notifs["workspace/didChangeWorkspaceFolders"].as_notification_handler()(params2);

    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws2_uri)));
    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws3_uri)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("FourDrive"), ::testing::StrEq(ws4_uri)));
    auto params3 = nlohmann::json::parse(R"({"event":{"added":[{"uri":")" + ws4_uri
        + R"(","name":"FourDrive"}],"removed":[{"uri":")" + ws2_uri + R"(","name":"TwoDrive"},{"uri":")" + ws3_uri
        + R"(","name":"ThreeDrive"}]}})");
    notifs["workspace/didChangeWorkspaceFolders"].as_notification_handler()(params3);
}

TEST(workspace_folders, did_change_watchedfiles_invalid_uri)
{
    test::ws_mngr_mock ws_mngr;
    response_provider_mock rpm;
    lsp::feature_workspace_folders f(ws_mngr, rpm);

    std::map<std::string, method> notifs;

    f.register_methods(notifs);
    notifs["workspace/didChangeWatchedFiles"].as_notification_handler()(
        R"({"changes":[{"uri":"user_storage:/user/storage/layout","type":2}, {"uri":"file:///file_name"}]})"_json);

    // If server didn't crash - hurray!
}

TEST(workspace_folders, initialize_folders)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    response_provider_mock rpm;
    lsp::feature_workspace_folders f(ws_mngr, rpm);

    EXPECT_CALL(rpm, request(std::string("client/registerCapability"), _, _, _)).Times(5);
    EXPECT_CALL(rpm, request(std::string("workspace/configuration"), _, _, _)).Times(5);

    // workspace folders on, but no workspaces provided
    auto init1 = R"({"processId":5236,
                     "rootPath":null,
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off",
                     "workspaceFolders":null})"_json;

    EXPECT_CALL(ws_mngr, add_workspace(_, _)).Times(0);

    f.initialize_feature(init1);
    f.initialized();


    // workspace folders on, two workspaces provided
    auto init2 = nlohmann::json::parse(R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\dont_load",
                     "rootUri":"file:///c%3A/Users/error","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off",
                     "workspaceFolders":[{"uri":")"
        + ws1_uri + R"(","name":"one"},{"uri":")" + ws2_uri + R"(","name":"two"}]})");

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("one"), ::testing::StrEq(ws1_uri)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("two"), ::testing::StrEq(ws2_uri)));
    f.initialize_feature(init2);
    f.initialized();

    // workspace folders off
    auto init3 = nlohmann::json::parse(R"({"processId":11244,
                     "rootPath":"c:\\Users\\error",
                     "rootUri":")"
        + ws1_uri
        + R"(","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_uri)));
    f.initialize_feature(init3);
    f.initialized();

    // fallback to rootPath
    auto init4 = nlohmann::json::parse(R"({"processId":11244,
                     "rootPath":")"
        + ws1_path_json_string + R"(",
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_uri)));
    f.initialize_feature(init4);
    f.initialized();

    // no rootUri provided (older version of LSP)
    auto init5 = nlohmann::json::parse(R"({"processId":11244,
                     "rootPath":")"
        + ws1_path_json_string
        + R"(","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_uri)));
    f.initialize_feature(init5);
    f.initialized();
}

TEST(workspace_folders, initialize_folder_with_configuration)
{
    using namespace ::testing;
    auto ws_mngr = parser_library::create_workspace_manager();
    NiceMock<workspace_manager_requests_mock> req_mock;
    response_provider_mock rpm;
    lsp::feature_workspace_folders f(*ws_mngr, rpm);

    ws_mngr->set_request_interface(&req_mock);

    EXPECT_CALL(rpm, request(std::string("workspace/configuration"), _, _, _)).Times(1);

    parser_library::workspace_manager_response<std::string_view> json_text;
    EXPECT_CALL(req_mock, request_workspace_configuration(StrEq(ws1_uri), _))
        .WillOnce(WithArg<1>([p = &json_text](auto&& x) { *p = std::move(x); }));

    auto init = nlohmann::json::parse(R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\dont_load",
                     "rootUri":"file:///c%3A/Users/error","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":false},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off",
                     "workspaceFolders":[{"uri":")"
        + ws1_uri + R"(","name":"one"}]})");

    f.initialize_feature(init);
    f.initialized();

    EXPECT_NO_THROW(json_text.provide("{}"));
}

TEST(workspace_folders, did_change_configuration)
{
    using namespace lsp;
    test::ws_mngr_mock ws_mngr;

    response_provider_mock provider;
    feature_workspace_folders feat(ws_mngr, provider);


    std::map<std::string, method> methods;
    feat.register_methods(methods);


    std::function<void(const nlohmann::json& params)> handler;
    nlohmann::json config_request_args {
        {
            "items",
            nlohmann::json::array_t {
                {
                    { "section", "hlasm" },
                },
            },
        },
    };

    EXPECT_CALL(provider, request("workspace/configuration", config_request_args, ::testing::_, ::testing::_))
        .WillOnce(::testing::SaveArg<2>(&handler));

    methods["workspace/didChangeConfiguration"].as_notification_handler()("{}"_json);

    parser_library::lib_config expected_config;
    expected_config.diag_supress_limit = 42;

    EXPECT_CALL(ws_mngr, configuration_changed(::testing::Eq(expected_config)));

    handler(R"([{"diagnosticsSuppressLimit":42}])"_json);
}

TEST(workspace_folders, did_change_configuration_with_requests)
{
    using namespace ::testing;

    auto ws_mngr = parser_library::create_workspace_manager();
    NiceMock<workspace_manager_requests_mock> req_mock;

    ws_mngr->add_workspace("test", "testurl");

    ws_mngr->set_request_interface(&req_mock);

    EXPECT_CALL(req_mock, request_workspace_configuration(StrEq("testurl"), _));

    ws_mngr->configuration_changed({});
}

TEST(workspace_folders, did_change_configuration_empty_configuration_params)
{
    using namespace lsp;

    test::ws_mngr_mock ws_mngr;

    response_provider_mock provider;
    feature_workspace_folders feat(ws_mngr, provider);


    std::map<std::string, method> methods;
    feat.register_methods(methods);


    std::function<void(const nlohmann::json& params)> handler;
    nlohmann::json config_request_args {
        {
            "items",
            nlohmann::json::array_t {
                { { "section", "hlasm" } },
            },
        },
    };

    EXPECT_CALL(provider, request("workspace/configuration", config_request_args, ::testing::_, ::testing::_))
        .WillOnce(::testing::SaveArg<2>(&handler));

    methods["workspace/didChangeConfiguration"].as_notification_handler()("{}"_json);

    EXPECT_CALL(ws_mngr, configuration_changed(::testing::_)).Times(0);

    handler(R"([])"_json);
}
