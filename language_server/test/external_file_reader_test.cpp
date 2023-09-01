/*
 * Copyright (c) 2023 Broadcom.
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
#include "gtest/gtest.h"
#include "json_channel_mock.h"

#include "../../parser_library/test/workspace_manager_response_mock.h"
#include "external_file_reader.h"
#include "nlohmann/json.hpp"

using namespace ::testing;
using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::parser_library;

TEST(external_file_reader, filter_predicate)
{
    NiceMock<mock_json_sink> sink;
    external_file_reader reader(sink);

    auto predicate = reader.get_filtering_predicate();

    EXPECT_TRUE(predicate(R"({"method":"external_file_response"})"_json));
    EXPECT_FALSE(predicate(R"({"method":"external_file_response1"})"_json));
    EXPECT_FALSE(predicate(R"({"method":"external_file_request"})"_json));
}

TEST(external_file_reader, file_reading)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] =
        make_workspace_manager_response(std::in_place_type<NiceMock<workspace_manager_response_mock<sequence<char>>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"read_file",
    "url":"AAA"
  }
})"_json));

    reader.read_external_file("AAA", r);

    EXPECT_CALL(*resp, provide(Truly([](sequence<char> v) { return std::string_view(v) == "AAACONTENT"; })));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":"AAACONTENT"
  }
})"_json);
}

TEST(external_file_reader, file_reading_bad)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] =
        make_workspace_manager_response(std::in_place_type<NiceMock<workspace_manager_response_mock<sequence<char>>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"read_file",
    "url":"AAA"
  }
})"_json));

    reader.read_external_file("AAA", r);

    EXPECT_CALL(*resp, error(_, _));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":[]
  }
})"_json);
}

TEST(external_file_reader, file_reading_error)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] =
        make_workspace_manager_response(std::in_place_type<NiceMock<workspace_manager_response_mock<sequence<char>>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"read_file",
    "url":"BBB"
  }
})"_json));

    reader.read_external_file("BBB", r);

    EXPECT_CALL(*resp, error(1, StrEq("Error")));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "error":{
      "code":1,
      "msg":"Error"
    }
  }
})"_json);
}

TEST(external_file_reader, directory_reading)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] = make_workspace_manager_response(
        std::in_place_type<NiceMock<workspace_manager_response_mock<workspace_manager_external_directory_result>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"list_directory",
    "url":"CCC"
  }
})"_json));

    reader.read_external_directory("CCC", r);

    EXPECT_CALL(*resp, provide(Truly([](workspace_manager_external_directory_result v) {
        return v.member_urls.size() == 2 && std::string_view(v.member_urls.item(0)) == "A"
            && std::string_view(v.member_urls.item(1)) == "B";
    })));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":{
      "member_urls": [
         "A", "B"
      ]
    }
  }
})"_json);
}

TEST(external_file_reader, directory_reading_bad)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] = make_workspace_manager_response(
        std::in_place_type<NiceMock<workspace_manager_response_mock<workspace_manager_external_directory_result>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"list_directory",
    "url":"CCC"
  }
})"_json));

    reader.read_external_directory("CCC", r);

    EXPECT_CALL(*resp, error(_, _));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":""
  }
})"_json);
}

TEST(external_file_reader, directory_reading_bad2)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] = make_workspace_manager_response(
        std::in_place_type<NiceMock<workspace_manager_response_mock<workspace_manager_external_directory_result>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"list_directory",
    "url":"CCC"
  }
})"_json));

    reader.read_external_directory("CCC", r);

    EXPECT_CALL(*resp, error(_, _));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":[5]
  }
})"_json);
}

TEST(external_file_reader, directory_reading_bad3)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] = make_workspace_manager_response(
        std::in_place_type<NiceMock<workspace_manager_response_mock<workspace_manager_external_directory_result>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"list_directory",
    "url":"CCC"
  }
})"_json));

    reader.read_external_directory("CCC", r);

    EXPECT_CALL(*resp, error(_, _));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "data":{"members":[5]}
  }
})"_json);
}

TEST(external_file_reader, directory_reading_error)
{
    NiceMock<mock_json_sink> sink;
    MockFunction<void()> wakeup;

    auto [r, resp] = make_workspace_manager_response(
        std::in_place_type<NiceMock<workspace_manager_response_mock<workspace_manager_external_directory_result>>>);
    external_file_reader reader(sink);

    auto reg = reader.register_thread(wakeup.AsStdFunction());

    EXPECT_CALL(sink,
        write_rvr(
            R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_request",
  "params":{
    "id":1,
    "op":"list_directory",
    "url":"DDD"
  }
})"_json));

    reader.read_external_directory("DDD", r);

    EXPECT_CALL(*resp, error(-1, StrEq("-Error")));
    EXPECT_CALL(wakeup, Call());

    reader.write(R"(
{
  "jsonrpc": "2.0",
  "method":"external_file_response",
  "params":{
    "id":1,
    "error":{
      "code":-1,
      "msg":"-Error"
    }
  }
})"_json);
}
