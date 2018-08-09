#include "gmock/gmock.h"
#include "../src/server.h"
#include <memory>
using namespace HlasmPlugin;
using namespace HlasmLanguageServer;
using namespace jsonrpcpp;



class ServerTest : public testing::Test
{
public:
	Server s = Server();

	void SetUp() override {
		
		s.registerCallbacks(
			std::bind(&ServerTest::response, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&ServerTest::notify, this, std::placeholders::_1, std::placeholders::_2),
			std::bind(&ServerTest::responseError, this, std::placeholders::_1, std::placeholders::_2));
	}

	void response(ID id, Json & json)
	{
		calledResponse = true;
		responseJson = json;
		responseID = id;
	}

	void responseError(ID id, Error & json)
	{
		calledResponseError = true;
		responseErrorJson = json;
		responseErrorID = id;
	}

	void notify(const std::string& method, Json & json)
	{
		calledNotify = true;
		notifyJson = json;
		notifyMethod = method;
	}

	bool calledResponseError = false;
	bool calledResponse = false;
	bool calledNotify = false;

	Json responseJson;
	Error responseErrorJson;
	Json notifyJson;

	ID responseID;
	ID responseErrorID;
	std::string notifyMethod;
};

TEST_F(ServerTest, initialize)
{
	//this is json params actually sent by vscode LSP client
	Json j = R"({"processId":5236,"rootPath":null,"rootUri":null,"capabilities":{"workspace":{"applyEdit":true,"workspaceEdit":{"documentChanges":true},"didChangeConfiguration":{"dynamicRegistration":true},"didChangeWatchedFiles":{"dynamicRegistration":true},"symbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"executeCommand":{"dynamicRegistration":true},"configuration":true,"workspaceFolders":true},"textDocument":{"publishDiagnostics":{"relatedInformation":true},"synchronization":{"dynamicRegistration":true,"willSave":true,"willSaveWaitUntil":true,"didSave":true},"completion":{"dynamicRegistration":true,"contextSupport":true,"completionItem":{"snippetSupport":true,"commitCharactersSupport":true,"documentationFormat":["markdown","plaintext"],"deprecatedSupport":true,"preselectSupport":true},"completionItemKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25]}},"hover":{"dynamicRegistration":true,"contentFormat":["markdown","plaintext"]},"signatureHelp":{"dynamicRegistration":true,"signatureInformation":{"documentationFormat":["markdown","plaintext"]}},"definition":{"dynamicRegistration":true},"references":{"dynamicRegistration":true},"documentHighlight":{"dynamicRegistration":true},"documentSymbol":{"dynamicRegistration":true,"symbolKind":{"valueSet":[1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26]}},"codeAction":{"dynamicRegistration":true,"codeActionLiteralSupport":{"codeActionKind":{"valueSet":["","quickfix","refactor","refactor.extract","refactor.inline","refactor.rewrite","source","source.organizeImports"]}}},"codeLens":{"dynamicRegistration":true},"formatting":{"dynamicRegistration":true},"rangeFormatting":{"dynamicRegistration":true},"onTypeFormatting":{"dynamicRegistration":true},"rename":{"dynamicRegistration":true},"documentLink":{"dynamicRegistration":true},"typeDefinition":{"dynamicRegistration":true},"implementation":{"dynamicRegistration":true},"colorProvider":{"dynamicRegistration":true}}},"trace":"off","workspaceFolders":null})"_json;

	auto a = std::make_shared<Request>(ID(5), "initialize", Parameter(j));
	s.callMethod(a);

	ASSERT_TRUE(calledResponse);
	ASSERT_FALSE(calledResponseError);

	EXPECT_EQ(responseID.type, ID::value_t::integer);
	EXPECT_EQ(responseID.int_id, ID(5).int_id);
	EXPECT_NE(responseJson.find("capabilities"), responseJson.end());

}
