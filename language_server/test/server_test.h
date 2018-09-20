#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H

#include <memory>

#include "gmock/gmock.h"

#include "../src/server.h"
#include "shared/workspace_manager.h"
#include "../src/feature_workspace_folders.h"
#include "../src/feature_text_synchronization.h"
#include "../src/feature.h"

#include "ws_mngr_mock.h"

using namespace hlasm_plugin;
using namespace language_server;
using namespace jsonrpcpp;

using json = nlohmann::json;

class server_test : public testing::Test
{
public:
	server s = server();

	void SetUp() override {
		
		s.register_callbacks(
			std::bind(&server_test::response, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&server_test::notify, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&server_test::response_error, this, std::placeholders::_1, std::placeholders::_2));
	}

	void response(id id, json & json)
	{
		called_response = true;
		response_json = json;
		response_id = id;
	}

	void response_error(id id, error & json)
	{
		called_response_error = true;
		response_error_json = json;
		response_error_id = id;
	}

	void notify(const std::string& method, json & json)
	{
		called_notify = true;
		notify_json = json;
		notify_method = method;
	}

	bool called_response_error = false;
	bool called_response = false;
	bool called_notify = false;

	json response_json;
	error response_error_json;
	json notify_json;

	id response_id;
	id response_error_id;
	std::string notify_method;
};

TEST_F(server_test, initialize)
{
	//this is json params actually sent by vscode LSP client
	json j = R"({"processId":5236,"rootPath":null,"rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,"workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off","workspaceFolders":null})"_json;

	auto a = std::make_shared<Request>(id(5), "initialize", parameter(j));
	s.call_method(a);

	ASSERT_TRUE(called_response);
	ASSERT_FALSE(called_response_error);

	EXPECT_EQ(response_id.type, id::value_t::integer);
	EXPECT_EQ(response_id.int_id, id(5).int_id);
	EXPECT_NE(response_json.find("capabilities"), response_json.end());
}

#ifdef _WIN32

TEST(workspace_folders, did_change_workspace_folders)
{
	using namespace parser_library;
	ws_mngr_mock ws_mngr;
	feature_workspace_folders f(ws_mngr);

	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("OneDrive"), ::testing::StrEq("c:\\path\\to\\W S\\OneDrive")));

	std::map<std::string, notification> notifs;

	f.register_notifications(notifs);

	json params1 = R"({"event":{"added":[{"uri":"file:///c%3A/path/to/W%20S/OneDrive","name":"OneDrive"}],"removed":[]}})"_json;

	notifs["workspace/didChangeWorkspaceFolders"](params1);

	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("TwoDrive"), ::testing::StrEq("c:\\path\\to\\W S\\TwoDrive")));
	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("ThreeDrive"), ::testing::StrEq("c:\\path\\to\\W S\\ThreeDrive")));
	EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq("c:\\path\\to\\W S\\OneDrive")));

	json params2 = R"({"event":{"added":[{"uri":"file:///c%3A/path/to/W%20S/TwoDrive","name":"TwoDrive"},{"uri":"file:///c%3A/path/to/W%20S/ThreeDrive","name":"ThreeDrive"}],"removed":[{"uri":"file:///c%3A/path/to/W%20S/OneDrive","name":"OneDrive"}]}})"_json;
	notifs["workspace/didChangeWorkspaceFolders"](params2);

	EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq("c:\\path\\to\\W S\\TwoDrive")));
	EXPECT_CALL(ws_mngr, remove_workspace(::testing::StrEq("c:\\path\\to\\W S\\ThreeDrive")));
	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("FourDrive"), ::testing::StrEq("c:\\path\\to\\W S\\FourDrive")));
	json params3 = R"({"event":{"added":[{"uri":"file:///c%3A/path/to/W%20S/FourDrive","name":"FourDrive"}],"removed":[{"uri":"file:///c%3A/path/to/W%20S/TwoDrive","name":"TwoDrive"},{"uri":"file:///c%3A/path/to/W%20S/ThreeDrive","name":"ThreeDrive"}]}})"_json;
	notifs["workspace/didChangeWorkspaceFolders"](params3);
}

TEST(workspace_folders, initialize_folders)
{
	using namespace parser_library;
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	feature_workspace_folders f(ws_mngr);

	//workspace folders on, but no workspaces provided
	json init1 = R"({"processId":5236,
                     "rootPath":null,
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off",
                     "workspaceFolders":null})"_json;

	EXPECT_CALL(ws_mngr, add_workspace(_, _)).Times(0);
	f.initialize_feature(init1);


	//workspace folders on, two workspaces provided
	json init2 = R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\dont_load",
                     "rootUri":"file:///c%3A/Users/error","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off",
                     "workspaceFolders":[{"uri":"file:///c%3A/Users/one","name":"one"},{"uri":"file:///c%3A/Users/two","name":"two"}]})"_json;
	
	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("one"), ::testing::StrEq("c:\\Users\\one")));
	EXPECT_CALL(ws_mngr, add_workspace(::testing::StrEq("two"), ::testing::StrEq("c:\\Users\\two")));
	f.initialize_feature(init2);

	//workspace folders off
	json init3 = R"({"processId":11244,
                     "rootPath":"c:\\Users\\error",
                     "rootUri":"file:///c%3A/Users/one","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})"_json;
	EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq("c:\\Users\\one")));
	f.initialize_feature(init3);

	//fallback to rootPath
	json init4 = R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\one",
                     "rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,
                     "workspaceFolders":false},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})"_json;
	EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq("c:\\Users\\Desktop\\one")));
	f.initialize_feature(init4);

	//no rootUri provided (older version of LSP)
	json init5 = R"({"processId":11244,
                     "rootPath":"c:\\Users\\Desktop\\one","capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]},"hierarchicalDocumentSymbolSupport":true},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true},"foldingRange":{"dynamicRegistration":true,"rangeLimit":5000,"lineFoldingOnly":true}}},"trace":"off"})"_json;
	EXPECT_CALL(ws_mngr, add_workspace(_, ::testing::StrEq("c:\\Users\\Desktop\\one")));
	f.initialize_feature(init5);
}

TEST(text_synchronization, did_open_file)
{
	using namespace parser_library;
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	feature_text_synchronization f(ws_mngr);
	std::map<std::string, notification> notifs;
	f.register_notifications(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","languageId":"plaintext","version":4,"text":"sad"}})"_json;
	
	EXPECT_CALL(ws_mngr, did_open_file(StrEq("c:\\test\\one\\blah.txt"), 4, StrEq("sad"), 3));
	
	notifs["textDocument/didOpen"](params1);
}

MATCHER_P2(PointerAndSizeEqArray, pointer, size, "") {
	(void)result_listener;
	const size_t actual_size = std::get<1>(arg);
	if (actual_size != size)
		return false;
	for (size_t i = 0; i < size; i++) {
		if (!(std::get<0>(arg)[i] == pointer[i]))
			return false;
	}
	return true;
}

TEST(text_synchronization, did_change_file)
{
	using namespace parser_library;
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	feature_text_synchronization f(ws_mngr);
	std::map<std::string, notification> notifs;
	f.register_notifications(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","version":7},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})"_json;

	document_change expected1[2]{ { { {0,0}, {0,8} }, "sad", 3 },  { { {1,12}, {1,14} }, "", 0 } };

	EXPECT_CALL(ws_mngr, did_change_file(StrEq("c:\\test\\one\\blah.txt"), 7, _, 2)).With(Args<2, 3>(PointerAndSizeEqArray(expected1, std::size(expected1))));
	notifs["textDocument/didChange"](params1);



	document_change expected2[1]{ { "sad", 3 } };
	json params2 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt","version":7},"contentChanges":[{"text":"sad"}]})"_json;
	EXPECT_CALL(ws_mngr, did_change_file(StrEq("c:\\test\\one\\blah.txt"), 7, _, 1)).With(Args<2, 3>(PointerAndSizeEqArray(expected2, std::size(expected2))));

	notifs["textDocument/didChange"](params2);



	json params3 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt"},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})"_json;

	EXPECT_THROW(notifs["textDocument/didChange"](params3), nlohmann::basic_json<>::exception);

}

TEST(text_synchronization, did_close_file)
{
	using namespace parser_library;
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	feature_text_synchronization f(ws_mngr);
	std::map<std::string, notification> notifs;
	f.register_notifications(notifs);

	json params1 = R"({"textDocument":{"uri":"file:///c%3A/test/one/blah.txt"}})"_json;
	EXPECT_CALL(ws_mngr, did_close_file(StrEq("c:\\test\\one\\blah.txt"))),

	notifs["textDocument/didClose"](params1);
}

TEST(feature, uri_to_path)
{
	using namespace language_server;
	EXPECT_EQ(feature::uri_to_path("file://czprfs50/Public"), "\\\\czprfs50\\Public");
	EXPECT_EQ(feature::uri_to_path("file:///C%3A/Public"), "C:\\Public");
}
#endif // _WIN32


#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_SERVER_TEST_H
