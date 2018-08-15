#include "gmock/gmock.h"
#include "../src/server.h"
#include <memory>
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
