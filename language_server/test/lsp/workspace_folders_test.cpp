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
#include "lsp/feature_workspace_folders.h"

using namespace hlasm_plugin::language_server;
#ifdef _WIN32
const std::string ws1_uri = "file:///c%3A/path/to/W%20S/OneDrive";
const std::string ws2_uri = "file:///c%3A/path/to/W%20S/TwoDrive";
const std::string ws3_uri = "file:///c%3A/path/to/W%20S/ThreeDrive";
const std::string ws4_uri = "file:///c%3A/path/to/W%20S/FourDrive";

const std::string ws1_path = R"(c:\path\to\W S\OneDrive)";
const std::string ws2_path = R"(c:\path\to\W S\TwoDrive)";
const std::string ws3_path = R"(c:\path\to\W S\ThreeDrive)";
const std::string ws4_path = R"(c:\path\to\W S\FourDrive)";

const std::string ws1_path_json_string = R"(c:\\path\\to\\W S\\OneDrive)";
#else
const std::string ws1_uri = "file:///path/to/W%20S/OneDrive";
const std::string ws2_uri = "file:///path/to/W%20S/TwoDrive";
const std::string ws3_uri = "file:///path/to/W%20S/ThreeDrive";
const std::string ws4_uri = "file:///path/to/W%20S/FourDrive";

const std::string ws1_path = R"(/path/to/W S/OneDrive)";
const std::string ws2_path = R"(/path/to/W S/TwoDrive)";
const std::string ws3_path = R"(/path/to/W S/ThreeDrive)";
const std::string ws4_path = R"(/path/to/W S/FourDrive)";

const std::string ws1_path_json_string = R"(/path/to/W S/OneDrive)";
#endif

TEST(workspace_folders, did_change_workspace_folders)
{
    ws_mngr_mock ws_mngr;
    response_provider_mock rpm;

    lsp::feature_workspace_folders f(ws_mngr, rpm);

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("OneDrive"), ::testing::StrEq(ws1_path)));

    std::map<std::string, method> notifs;

    f.register_methods(notifs);

    json params1 = json::parse(R"({"event":{"added":[{"uri":")" + ws1_uri + R"(","name":"OneDrive"}],"removed":[]}})");

    notifs["workspace/didChangeWorkspaceFolders"]("", params1);

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("TwoDrive"), ::testing::StrEq(ws2_path)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("ThreeDrive"), ::testing::StrEq(ws3_path)));
    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws1_path)));

    json params2 = json::parse(R"({"event":{"added":[{"uri":")" + ws2_uri + R"(","name":"TwoDrive"},{"uri":")" + ws3_uri
        + R"(","name":"ThreeDrive"}],"removed":[{"uri":")" + ws1_uri + R"(","name":"OneDrive"}]}})");
    notifs["workspace/didChangeWorkspaceFolders"]("", params2);

    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws2_path)));
    EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq(ws3_path)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("FourDrive"), ::testing::StrEq(ws4_path)));
    json params3 =
        json::parse(R"({"event":{"added":[{"uri":")" + ws4_uri + R"(","name":"FourDrive"}],"removed":[{"uri":")"
            + ws2_uri + R"(","name":"TwoDrive"},{"uri":")" + ws3_uri + R"(","name":"ThreeDrive"}]}})");
    notifs["workspace/didChangeWorkspaceFolders"]("", params3);
}

TEST(workspace_folders, initialize_folders)
{
    using namespace ::testing;
    ws_mngr_mock ws_mngr;
    response_provider_mock rpm;
    lsp::feature_workspace_folders f(ws_mngr, rpm);

    for (int config_request_number = 0; config_request_number < 5; ++config_request_number)
        EXPECT_CALL(rpm,
            request(json("config_request_" + std::to_string(config_request_number)),
                std::string("workspace/configuration"),
                _,
                _))
            .Times(1);

    // workspace folders on, but no workspaces provided
    json init1 = R"({"processId":5236,
                     "rootPath":null,
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off",
                     "workspaceFolders":null})"_json;

    EXPECT_CALL(ws_mngr, add_workspace(_, _)).Times(0);

    f.initialize_feature(init1);


    // workspace folders on, two workspaces provided
    json init2 = json::parse(R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\dont_load",
                     "rootUri":"file:///c%3A/Users/error","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off",
                     "workspaceFolders":[{"uri":")"
        + ws1_uri + R"(","name":"one"},{"uri":")" + ws2_uri + R"(","name":"two"}]})");

    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("one"), ::testing::StrEq(ws1_path)));
    EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("two"), ::testing::StrEq(ws2_path)));
    f.initialize_feature(init2);

    // workspace folders off
    json init3 = json::parse(R"({"processId":11244,
                     "rootPath":"c:\\Users\\error",
                     "rootUri":")"
        + ws1_uri
        + R"(","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_path)));
    f.initialize_feature(init3);

    // fallback to rootPath
    json init4 = json::parse(R"({"processId":11244,
                     "rootPath":")"
        + ws1_path_json_string + R"(",
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_path)));
    f.initialize_feature(init4);

    // no rootUri provided (older version of LSP)
    json init5 = json::parse(R"({"processId":11244,
                     "rootPath":")"
        + ws1_path_json_string
        + R"(","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})");
    EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq(ws1_path)));
    f.initialize_feature(init5);
}
