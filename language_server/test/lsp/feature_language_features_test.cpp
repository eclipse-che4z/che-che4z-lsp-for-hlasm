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

#ifndef HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_LANGUAGE_FEATURES_TEST_H
#define HLASMPLUGIN_LANGUAGESERVER_TEST_FEATURE_LANGUAGE_FEATURES_TEST_H

#include "gmock/gmock.h"

#include "../response_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "lsp/feature_language_features.h"
#include "utils/platform.h"

using hlasm_plugin::utils::platform::is_windows;

const char* path = is_windows() ? "c:\\test" : "/home/test";

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;

TEST(language_features, completion)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    json params1 = is_windows()
        ? R"({"textDocument":{"uri":"file:///c%3A/test"},"position":{"line":0,"character":1},"context":{"triggerKind":1}})"_json
        : R"({"textDocument":{"uri":"file:///home/test"},"position":{"line":0,"character":1},"context":{"triggerKind":1}})"_json;

    EXPECT_CALL(ws_mngr,
        completion(
            StrEq(path), parser_library::position(0, 1), '\0', parser_library::completion_trigger_kind::invoked));
    notifs["textDocument/completion"].handler("", params1);
}

TEST(language_features, hover)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    json params1 = is_windows()
        ? R"({"textDocument":{"uri":"file:///c%3A/test"},"position":{"line":0,"character":1}})"_json
        : R"({"textDocument":{"uri":"file:///home/test"},"position":{"line":0,"character":1}})"_json;

    std::string s("test");
    std::string_view ret(s);
    EXPECT_CALL(ws_mngr, hover(StrEq(path), parser_library::position(0, 1))).WillOnce(Return(ret));
    notifs["textDocument/hover"].handler("", params1);
}

TEST(language_features, definition)
{
    using namespace ::testing;


    parser_library::workspace_manager ws_mngr;

    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    json params1 = is_windows()
        ? R"({"textDocument":{"uri":"file:///c%3A/test"},"position":{"line":0,"character":1}})"_json
        : R"({"textDocument":{"uri":"file:///home/test"},"position":{"line":0,"character":1}})"_json;

    EXPECT_CALL(response_mock, respond(json(""), "", ::testing::_));
    notifs["textDocument/definition"].handler("", params1);
}

TEST(language_features, references)
{
    using namespace ::testing;
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    json params1 = is_windows()
        ? R"({"textDocument":{"uri":"file:///c%3A/test"},"position":{"line":0,"character":1}})"_json
        : R"({"textDocument":{"uri":"file:///home/test"},"position":{"line":0,"character":1}})"_json;

    EXPECT_CALL(ws_mngr, references(StrEq(path), parser_library::position(0, 1)));
    notifs["textDocument/references"].handler("", params1);
}

TEST(language_features, document_symbol)
{
    using namespace ::testing;
    parser_library::workspace_manager ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "A EQU 1";
    ws_mngr.did_open_file("test", 0, file_text.c_str(), file_text.size());
    json params1 = json::parse(R"({"textDocument":{"uri":")" + feature::path_to_uri("test") + "\"}}");

    json r = { { "start", { { "line", 0 }, { "character", 0 } } }, { "end", { { "line", 0 }, { "character", 0 } } } };
    json response = json::array();
    response.push_back(
        { { "name", "A" }, { "kind", 17 }, { "range", r }, { "selectionRange", r }, { "children", json::array() } });
    EXPECT_CALL(response_mock, respond(json(""), std::string(""), response));
    notifs["textDocument/documentSymbol"].handler("", params1);
}

TEST(language_features, semantic_tokens)
{
    using namespace ::testing;
    parser_library::workspace_manager ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "A EQU 1\n SAM31";
    ws_mngr.did_open_file("test", 0, file_text.c_str(), file_text.size());
    json params1 = json::parse(R"({"textDocument":{"uri":")" + feature::path_to_uri("test") + "\"}}");

    json response { { "data", { 0, 0, 1, 0, 0, 0, 2, 3, 1, 0, 0, 4, 1, 10, 0, 1, 1, 5, 1, 0 } } };
    EXPECT_CALL(response_mock, respond(json(""), std::string(""), response));

    notifs["textDocument/semanticTokens/full"].handler("", params1);
}

TEST(language_features, semantic_tokens_multiline)
{
    using namespace ::testing;
    parser_library::workspace_manager ws_mngr;
    response_provider_mock response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = R"(
D EQU                                                                 1X3145
IIIIIIIIIIIIIII1
)";

    ws_mngr.did_open_file("test", 0, file_text.c_str(), file_text.size());
    json params1 = json::parse(R"({"textDocument":{"uri":")" + feature::path_to_uri("test") + "\"}}");

    // clang-format off
    json response { { "data",
        { 1,0,1,0,0,      // label         D
            0,2,3,1,0,    // instruction   EQU
            0,68,1,10,0,  // number        1
            0,1,1,5,0,    // continuation  X
            0,1,4,3,0,    // ignored       3145
            1,0,15,3,0,   // ignored       IIIIIIIIIIIIIII
            0,15,1,10,0   // number        1
        } } };
    // clang-format on
    EXPECT_CALL(response_mock, respond(json(""), std::string(""), response));

    notifs["textDocument/semanticTokens/full"].handler("", params1);
}


namespace {
struct test_param
{
    std::string name;
    std::vector<parser_library::token_info> tokens;
    json result;
};

struct stringer
{
    std::string operator()(::testing::TestParamInfo<test_param> p) { return p.param.name; }
};

class convert_tokens_fixture : public ::testing::TestWithParam<test_param>
{};

} // namespace

INSTANTIATE_TEST_SUITE_P(convert_tokens_to_num_array,
    convert_tokens_fixture,
    ::testing::Values(test_param { "empty", {}, json::array() },
        test_param { "one_token",
            {
                { { { 0, 0 }, { 0, 5 } }, parser_library::semantics::hl_scopes::instruction },
            },
            { 0, 0, 5, 1, 0 } },
        test_param {
            "one_side_overlap",
            {
                { { { 0, 0 }, { 0, 5 } }, parser_library::semantics::hl_scopes::instruction },
                { { { 0, 3 }, { 0, 8 } }, parser_library::semantics::hl_scopes::remark },
                { { { 0, 10 }, { 0, 11 } }, parser_library::semantics::hl_scopes::ignored },
            },
            // clang-format off
            { 0, 0, 3, 1, 0,
              0, 3, 5, 2, 0,
              0, 7, 1, 3, 0
              },
            // clang-format on
        },
        test_param {
            "enclosed_overlap",
            {
                { { { 0, 0 }, { 0, 10 } }, parser_library::semantics::hl_scopes::string },
                { { { 0, 1 }, { 0, 3 } }, parser_library::semantics::hl_scopes::var_symbol },
                { { { 0, 5 }, { 0, 7 } }, parser_library::semantics::hl_scopes::var_symbol },
                { { { 0, 7 }, { 0, 9 } }, parser_library::semantics::hl_scopes::var_symbol },
                { { { 0, 10 }, { 0, 11 } }, parser_library::semantics::hl_scopes::ignored },
            },
            // clang-format off
            { 0, 0, 1, 9, 0,   // beginning of string
              0, 1, 2, 7, 0,   // 1. var symbol
              0, 2, 2, 9, 0,   // string between 1. and 2. var symbol
              0, 2, 2, 7, 0,   // 2. var symbol
              0, 2, 2, 7, 0,   // 3. var symbol
              0, 2, 1, 9, 0,   // ending of string
              0, 1, 1, 3, 0
              },
            // clang-format on
        },
        test_param {
            // Case like this probably never happens from the real grammar, but the code should be able to handle it.
            "nested_enclosed_overlap",
            {
                { { { 0, 0 }, { 0, 15 } }, parser_library::semantics::hl_scopes::string },
                { { { 0, 1 }, { 0, 3 } }, parser_library::semantics::hl_scopes::var_symbol },
                { { { 0, 4 }, { 0, 10 } }, parser_library::semantics::hl_scopes::instruction },
                { { { 0, 5 }, { 0, 8 } }, parser_library::semantics::hl_scopes::var_symbol },
                { { { 0, 11 }, { 0, 14 } }, parser_library::semantics::hl_scopes::var_symbol },
            },
            // clang-format off
            { 0, 0, 1, 9, 0,   // beginning of string
              0, 1, 2, 7, 0,   // 1. var symbol
              0, 2, 1, 9, 0,   // string between 1. var symbol and instruction
              0, 1, 1, 1, 0,   // beginning of instruction
              0, 1, 3, 7, 0,   // 2. var symbol
              0, 3, 2, 1, 0,   // end of instruction
              0, 2, 1, 9, 0,   // string 
              0, 1, 3, 7, 0,   // 3. var symbol
              0, 3, 1, 9, 0    // string
              },
            // clang-format on
        }),
    stringer());



TEST_P(convert_tokens_fixture, test)
{
    using namespace lsp;
    using namespace parser_library;
    std::vector<token_info> tokens;
    json result = feature_language_features::convert_tokens_to_num_array(GetParam().tokens);
    EXPECT_EQ(result, GetParam().result);
}

#endif
