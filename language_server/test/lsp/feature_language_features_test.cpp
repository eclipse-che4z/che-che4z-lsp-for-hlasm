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

#include "gmock/gmock.h"

#include "../response_provider_mock.h"
#include "../ws_mngr_mock.h"
#include "completion_item.h"
#include "lsp/feature_language_features.h"
#include "nlohmann/json.hpp"
#include "utils/platform.h"
#include "utils/resource_location.h"

using hlasm_plugin::utils::platform::is_windows;

const std::string uri = is_windows() ? "file:///c%3A/test" : "file:///home/test";

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace ::testing;

TEST(language_features, completion)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + uri + R"("},"position":{"line":0,"character":1},"context":{"triggerKind":1}})");

    EXPECT_CALL(ws_mngr,
        completion(
            StrEq(uri), parser_library::position(0, 1), '\0', parser_library::completion_trigger_kind::invoked, _));
    notifs["textDocument/completion"].as_request_handler()(request_id(0), params1);
}

TEST(language_features, completion_resolve)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + uri + R"("},"position":{"line":0,"character":1},"context":{"triggerKind":1}})");

    static const std::vector compl_list {
        hlasm_plugin::parser_library::completion_item("LABEL", "", "", "DOC"),
    };
    EXPECT_CALL(ws_mngr,
        completion(
            StrEq(uri), parser_library::position(0, 1), '\0', parser_library::completion_trigger_kind::invoked, _))
        .WillOnce(WithArg<4>(Invoke([](auto channel) { channel.provide(compl_list); })));
    nlohmann::json completion_response;
    EXPECT_CALL(response_mock, respond(request_id(0), StrEq(""), _)).WillOnce(SaveArg<2>(&completion_response));
    notifs["textDocument/completion"].as_request_handler()(request_id(0), params1);
    EXPECT_EQ(completion_response.at("/items/0/label"_json_pointer), "LABEL");
    EXPECT_FALSE(completion_response.contains("/items/0/documentation"_json_pointer));

    nlohmann::json completion_resolve_response;
    EXPECT_CALL(response_mock, respond(request_id(1), StrEq(""), _)).WillOnce(SaveArg<2>(&completion_resolve_response));
    notifs["completionItem/resolve"].as_request_handler()(request_id(1), nlohmann::json { { "label", "LABEL" } });
    EXPECT_EQ(completion_resolve_response.at("/label"_json_pointer), "LABEL");
    EXPECT_EQ(completion_resolve_response.at("/documentation/value"_json_pointer), "DOC");
}

TEST(language_features, completion_resolve_invalid)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    nlohmann::json completion_resolve_response;
    EXPECT_CALL(response_mock, respond(request_id(1), StrEq(""), _)).WillOnce(SaveArg<2>(&completion_resolve_response));
    notifs["completionItem/resolve"].as_request_handler()(request_id(1), nlohmann::json { { "label", "LABEL" } });
    EXPECT_EQ(completion_resolve_response.at("/label"_json_pointer), "LABEL");
    EXPECT_EQ(completion_resolve_response.at("/documentation"_json_pointer), "");
}

TEST(language_features, hover)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + uri + R"("},"position":{"line":0,"character":1},"context":{"triggerKind":1}})");

    std::string s("test");
    EXPECT_CALL(ws_mngr, hover(StrEq(uri), parser_library::position(0, 1), _));
    notifs["textDocument/hover"].as_request_handler()(request_id(0), params1);
}

TEST(language_features, definition)
{
    auto ws_mngr = parser_library::create_workspace_manager();

    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + uri + R"("},"position":{"line":0,"character":1},"context":{"triggerKind":1}})");

    EXPECT_CALL(response_mock, respond(request_id(0), "", _));
    notifs["textDocument/definition"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, references)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(
        R"({"textDocument":{"uri":")" + uri + R"("},"position":{"line":0,"character":1},"context":{"triggerKind":1}})");

    EXPECT_CALL(ws_mngr, references(StrEq(uri), parser_library::position(0, 1), _));
    notifs["textDocument/references"].as_request_handler()(request_id(0), params1);
}

TEST(language_features, document_symbol)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "A EQU 1";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json r = {
        { "start", { { "line", 0 }, { "character", 0 } } },
        { "end", { { "line", 0 }, { "character", 2147483647 } } },
    };
    nlohmann::json response = nlohmann::json::array();
    response.push_back({
        { "name", "A" },
        { "kind", 14 },
        { "range", r },
        { "selectionRange", r },
        { "children", nlohmann::json::array() },
    });
    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));
    notifs["textDocument/documentSymbol"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, semantic_tokens)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "A EQU 1\n SAM31";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json response { { "data", { 0, 0, 1, 0, 0, 0, 2, 3, 1, 0, 0, 4, 1, 10, 0, 1, 1, 5, 1, 0 } } };
    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));

    notifs["textDocument/semanticTokens/full"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, semantic_tokens_cancelled)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "A EQU 1\n SAM31";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json response { { "data", { 0, 0, 1, 0, 0, 0, 2, 3, 1, 0, 0, 4, 1, 10, 0, 1, 1, 5, 1, 0 } } };

    std::function<void()> request_invalidator;
    EXPECT_CALL(response_mock, register_cancellable_request(request_id(0), _))
        .WillOnce(WithArg<1>([&request_invalidator](auto x) { request_invalidator = x.take_invalidator(); }));
    notifs["textDocument/semanticTokens/full"].as_request_handler()(request_id(0), params1);
    ASSERT_TRUE(request_invalidator);

    request_invalidator();

    EXPECT_CALL(response_mock, respond_error(request_id(0), _, -32800, "Canceled", _));
    ws_mngr->idle_handler();
}

TEST(language_features, semantic_tokens_multiline)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = R"(
D EQU                                                                 1X3145
IIIIIIIIIIIIIII1
)";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    // clang-format off
    nlohmann::json response { { "data",
        { 1,0,1,0,0,      // label         D
            0,2,3,1,0,    // instruction   EQU
            0,68,1,10,0,  // number        1
            0,1,1,5,0,    // continuation  X
            0,1,4,3,0,    // ignored       3145
            1,0,15,3,0,   // ignored       IIIIIIIIIIIIIII
            0,15,1,10,0   // number        1
        } } };
    // clang-format on
    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));

    notifs["textDocument/semanticTokens/full"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, semantic_tokens_multiline_overlap)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = R"(
&X SETC ' '
.X AIF ('&X' EQ '&X').Y
.Y ANOP
)";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    // clang-format off
    nlohmann::json response { { "data",
        {   1,0,2,7,0,    // var symbol    &X
            0,3,4,1,0,    // instruction   SETC
            0,5,3,9,0,    // string        ' '
            1,0,2,6,0,    // seq symbol    .X
            0,3,3,1,0,    // instr         AIF
            0,4,1,8,0,    // operator      (
            0,1,1,9,0,    // string begin  '
            0,1,2,7,0,    // var sym       &X
            0,2,1,9,0,    // string end    '
            0,2,2,11,0,   // operand       EQ
            0,3,1,9,0,    // string begin  '
            0,1,2,7,0,    // var sym       &X
            0,2,1,9,0,    // string end    '
            0,1,1,8,0,    // operator      )
            0,1,2,6,0,    // seq symbol    .Y
            1,0,2,6,0,    // seq symbol    .Y
            0,3,4,1,0     // instruction   ANOP
        } } };
    // clang-format on
    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));

    notifs["textDocument/semanticTokens/full"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}



namespace {
struct test_param
{
    std::string name;
    std::vector<parser_library::token_info> tokens;
    nlohmann::json result;
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
    ::testing::Values(test_param { "empty", {}, nlohmann::json::array() },
        test_param { "one_token",
            {
                { { { 0, 0 }, { 0, 5 } }, parser_library::hl_scopes::instruction },
            },
            { 0, 0, 5, 1, 0 } },
        test_param {
            "one_side_overlap",
            {
                { { { 0, 0 }, { 0, 5 } }, parser_library::hl_scopes::instruction },
                { { { 0, 3 }, { 0, 8 } }, parser_library::hl_scopes::remark },
                { { { 0, 10 }, { 0, 11 } }, parser_library::hl_scopes::ignored },
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
                { { { 0, 0 }, { 0, 10 } }, parser_library::hl_scopes::string },
                { { { 0, 1 }, { 0, 3 } }, parser_library::hl_scopes::var_symbol },
                { { { 0, 5 }, { 0, 7 } }, parser_library::hl_scopes::var_symbol },
                { { { 0, 7 }, { 0, 9 } }, parser_library::hl_scopes::var_symbol },
                { { { 0, 10 }, { 0, 11 } }, parser_library::hl_scopes::ignored },
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
                { { { 0, 0 }, { 0, 15 } }, parser_library::hl_scopes::string },
                { { { 0, 1 }, { 0, 3 } }, parser_library::hl_scopes::var_symbol },
                { { { 0, 4 }, { 0, 10 } }, parser_library::hl_scopes::instruction },
                { { { 0, 5 }, { 0, 8 } }, parser_library::hl_scopes::var_symbol },
                { { { 0, 11 }, { 0, 14 } }, parser_library::hl_scopes::var_symbol },
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
    nlohmann::json result = feature_language_features::convert_tokens_to_num_array(GetParam().tokens);
    EXPECT_EQ(result, GetParam().result);
}

TEST(language_features, opcode_suggestion)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 =
        nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + R"("},"opcodes":["LHIXXX"],"extended":false})");

    using namespace hlasm_plugin::parser_library;

    auto expected_json =
        nlohmann::json::parse(R"({"uri":")" + uri + R"(","suggestions":{"LHIXXX":[{"opcode":"LHI","distance":3}]}})");
    EXPECT_CALL(response_mock, respond(_, _, std::move(expected_json)));

    notifs["textDocument/$/opcode_suggestion"].as_request_handler()(request_id(0), params1);
}

TEST(language_features, branch_information)
{
    test::ws_mngr_mock ws_mngr;
    NiceMock<response_provider_mock> response_mock;
    lsp::feature_language_features f(ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    auto params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + R"("}})");

    using namespace hlasm_plugin::parser_library;

    EXPECT_CALL(ws_mngr, branch_information(StrEq(uri), _)).WillOnce(WithArg<1>(Invoke([](auto channel) {
        channel.provide(std::vector<branch_info> {
            branch_info(1, 2, branch_direction::up),
            branch_info(3, 4, branch_direction::down),
            branch_info(5, 6, branch_direction::somewhere),
        });
    })));

    auto expected_json = nlohmann::json::parse(R"(
[
  {
    "line": 1,
    "col": 2,
    "up": true,
    "down": false,
    "somewhere": false
  },
  {
    "line": 3,
    "col": 4,
    "up": false,
    "down": true,
    "somewhere": false
  },
  {
    "line": 5,
    "col": 6,
    "up": false,
    "down": false,
    "somewhere": true
  }
]
)");
    EXPECT_CALL(response_mock, respond(_, _, std::move(expected_json)));

    notifs["textDocument/$/branch_information"].as_request_handler()(request_id(0), params1);
}

TEST(language_features, folding)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "*\n*\n*\n";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json response = nlohmann::json::array();
    response.push_back({
        { "startLine", 0 },
        { "endLine", 2 },
        { "kind", "comment" },
    });

    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));
    notifs["textDocument/foldingRange"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, retrieve_output)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = " PUNCH 'A'\n MNOTE 2,'B'";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json response = nlohmann::json::array();
    response.push_back({
        { "level", -1 },
        { "text", "A" },
    });
    response.push_back({
        { "level", 2 },
        { "text", "B" },
    });

    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));
    notifs["textDocument/$/retrieve_outputs"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}

TEST(language_features, retrieve_output_empty)
{
    auto ws_mngr = parser_library::create_workspace_manager();
    response_provider_mock response_mock;
    lsp::feature_language_features f(*ws_mngr, response_mock);
    std::map<std::string, method> notifs;
    f.register_methods(notifs);

    std::string file_text = "";

    ws_mngr->did_open_file(uri, 0, file_text);
    nlohmann::json params1 = nlohmann::json::parse(R"({"textDocument":{"uri":")" + uri + "\"}}");

    nlohmann::json response = nlohmann::json::array();

    EXPECT_CALL(response_mock, respond(request_id(0), std::string(""), std::move(response)));
    notifs["textDocument/$/retrieve_outputs"].as_request_handler()(request_id(0), params1);

    ws_mngr->idle_handler();
}
