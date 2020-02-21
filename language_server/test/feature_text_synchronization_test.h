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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H

#include "../src/lsp/feature_text_synchronization.h"

#ifdef _WIN32
const std::string txt_file_uri = R"(file:///c%3A/test/one/blah.txt)";
const std::string txt_file_path = R"(c:\test\one\blah.txt)";
#else
const std::string txt_file_uri = R"(file:///home/user/somefile)";
const std::string txt_file_path = R"(/home/user/somefile)";
#endif

TEST(text_synchronization, did_open_file)
{
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"(","languageId":"plaintext","version":4,"text":"sad"}})");

	EXPECT_CALL(ws_mngr, did_open_file(StrEq(txt_file_path), 4, StrEq("sad"), 3));

	notifs["textDocument/didOpen"]("", params1);
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
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"(","version":7},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})");

	document_change expected1[2]{ { { {0,0}, {0,8} }, "sad", 3 },  { { {1,12}, {1,14} }, "", 0 } };

	EXPECT_CALL(ws_mngr, did_change_file(StrEq(txt_file_path), 7, _, 2)).With(Args<2, 3>(PointerAndSizeEqArray(expected1, std::size(expected1))));
	notifs["textDocument/didChange"]("",params1);



	document_change expected2[1]{ { "sad", 3 } };
	json params2 = json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"(","version":7},"contentChanges":[{"text":"sad"}]})");
	EXPECT_CALL(ws_mngr, did_change_file(StrEq(txt_file_path), 7, _, 1)).With(Args<2, 3>(PointerAndSizeEqArray(expected2, std::size(expected2))));

	notifs["textDocument/didChange"]("",params2);



	json params3 = json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"("},"contentChanges":[{"range":{"start":{"line":0,"character":0},"end":{"line":0,"character":8}},"rangeLength":8,"text":"sad"}, {"range":{"start":{"line":1,"character":12},"end":{"line":1,"character":14}},"rangeLength":2,"text":""}]})");

	EXPECT_THROW(notifs["textDocument/didChange"]("",params3), nlohmann::basic_json<>::exception);

}

TEST(text_synchronization, did_close_file)
{
	using namespace ::testing;
	ws_mngr_mock ws_mngr;
	response_provider_mock response_mock;
	lsp::feature_text_synchronization f(ws_mngr, response_mock);
	std::map<std::string, method> notifs;
	f.register_methods(notifs);

	json params1 = json::parse(R"({"textDocument":{"uri":")" + txt_file_uri + R"("}})");
	EXPECT_CALL(ws_mngr, did_close_file(StrEq(txt_file_path))),

	notifs["textDocument/didClose"]("",params1);
}

#ifdef _WIN32

TEST(feature, uri_to_path)
{
	using namespace hlasm_plugin::language_server;
	EXPECT_EQ(feature::uri_to_path("file://czprfs50/Public"), "\\\\czprfs50\\Public");
	EXPECT_EQ(feature::uri_to_path("file:///C%3A/Public"), "c:\\Public");
}

TEST(feature, path_to_uri)
{
	using namespace hlasm_plugin::language_server;
	EXPECT_EQ(feature::path_to_uri("\\\\czprfs50\\Public"), "file://czprfs50/Public");
	EXPECT_EQ(feature::path_to_uri("c:\\Public"), "file:///c%3A/Public");
}
#else

TEST(feature, uri_to_path)
{
	using namespace hlasm_plugin::language_server;
	EXPECT_EQ(feature::uri_to_path("file:///home/user/somefile"), "/home/user/somefile");
	EXPECT_EQ(feature::uri_to_path("file:///C%3A/Public"), "/C:/Public");
}

TEST(feature, path_to_uri)
{
	using namespace hlasm_plugin::language_server;
	EXPECT_EQ(feature::path_to_uri("/home/user/somefile"), "file:///home/user/somefile");
	EXPECT_EQ(feature::path_to_uri("/C:/Public"), "file:///C%3A/Public");
}

#endif // _WIN32

#endif // !HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_TEXT_SYNCHRONIZATION_TEST_H