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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H

#include <memory>
#include "json.hpp"
#include "gmock/gmock.h"

#include "../src/lsp/lsp_server.h"
#include "shared/workspace_manager.h"
#include "../src/lsp/feature_workspace_folders.h"
#include "../src/lsp/feature_text_synchronization.h"
#include "../src/feature.h"


#include "send_message_provider_mock.h"
#include "ws_mngr_mock.h"

namespace nlohmann
{
//needed in order to have mock methods with json arguments
inline void PrintTo(json const& json, std::ostream* os)
{
	*os << json.dump();
}
}

using namespace hlasm_plugin;
using namespace language_server;

TEST(lsp_server_test, initialize)
{
	//this is json params actually sent by vscode LSP client
	json j = R"({"jsonrpc":"2.0","id":47,"method":"initialize","params":{"processId":5236,"rootPath":null,"rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,"workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off","workspaceFolders":null}})"_json;
	ws_mngr_mock ws_mngr;
	send_message_provider_mock smpm;
	lsp::server s(ws_mngr);
	s.set_send_message_provider(&smpm);
	
	

	json server_capab;

	json show_message = R"({"jsonrpc":"2.0", "method" : "window/showMessage", "params" : {"message":"The capabilities of hlasm language server were sent!", "type" : 3}})"_json;

	EXPECT_CALL(smpm, reply(::testing::_)).WillOnce(::testing::SaveArg<0>(&server_capab));
	EXPECT_CALL(smpm, reply(show_message)).Times(::testing::AtMost(1));
	s.message_received(j);

	

	
	EXPECT_NE(server_capab.find("jsonrpc"), server_capab.end());
	ASSERT_NE(server_capab.find("id"), server_capab.end());
	EXPECT_EQ(server_capab["id"].get<json::number_unsigned_t>(), 47);
	ASSERT_NE(server_capab.find("result"), server_capab.end());
	EXPECT_NE(server_capab["result"].find("capabilities"), server_capab["result"].end());
}

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H
