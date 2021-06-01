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

#include <sstream>

#include "gmock/gmock.h"

#include "base_protocol_channel.h"
#include "dispatcher.h"
#include "stream_helper.h"


using namespace hlasm_plugin::language_server;

class server_mock : public server
{
    hlasm_plugin::parser_library::workspace_manager ws;
    int counter = 0;
    int messages_limit;

public:
    server_mock(int max_messages)
        : server(ws)
        , messages_limit(max_messages)
    {}

    std::vector<json> messages;

    void request(const json&, const std::string&, const json&, method) override {}
    void respond(const json&, const std::string&, const json&) override {}
    void notify(const std::string&, const json&) override {}
    void respond_error(const json&, const std::string&, int, const std::string&, const json&) override {}
    void message_received(const json& message) override
    {
        ++counter;
        if (counter == messages_limit)
        {
            shutdown_request_received_ = true;
            exit_notification_received_ = true;
        }
        messages.push_back(message);
    }
};

struct test_param
{
    int messages_limit;
    int return_value;
    std::vector<std::string> headers;
    std::vector<json> messages;
    bool write_messages;
    std::string name;
};

struct stringer
{
    std::string operator()(::testing::TestParamInfo<test_param> p) { return p.param.name; }
};

class dispatcher_fixture : public ::testing::TestWithParam<test_param>
{};

INSTANTIATE_TEST_SUITE_P(dispatcher,
    dispatcher_fixture,
    ::testing::Values(
        test_param { 1, 1, { "Content-Length: 30\r\n\r\n\"A json message 1\"" }, {}, false, "unexpected_eof" },
        test_param { 1,
            0,
            { "Content-Length: 18\r\nShould be ignored\r\n\r\n\"A json message 1\"" },
            { "\"A json message 1\""_json },
            false,
            "unexpected_header_entry" },
        test_param { 1,
            0,
            { "Content-Length: 30\r\nContent-Length: 18\r\n\r\n" },
            { "\"A json message 1\""_json },
            true,
            "two_content_length_headers" },
        test_param { 1,
            0,
            { "Content-Length: 0\r\nContent-Length: 18\r\n\r\n" },
            { "\"A json message 1\""_json },
            true,
            "content_length_zero" },
        test_param { 1,
            1,
            { "Content-Length: 3000000000000000000\r\n\r\n\"A json message 1\"" },
            {},
            false,
            "unexpected_content_length" },
        test_param {
            1, 1, { "Content-Length: 27\r\n\r\n\"A malformed json message 1" }, {}, false, "malformed_message" },
        test_param { 2,
            0,
            { "Content-Length: 18\r\n\r\n", "Content-Length: 20\r\n\r\n" },
            { "\"A json message 1\""_json, R"({"Second":"message"})"_json },
            true,
            "two_messages" }),
    stringer());

TEST_P(dispatcher_fixture, basic)
{
    std::stringstream ss_in;
    std::stringstream ss_out;
    for (size_t i = 0; i < GetParam().headers.size(); ++i)
    {
        ss_in << GetParam().headers[i];
        if (GetParam().write_messages)
            ss_in << GetParam().messages[i].dump();
    }

    newline_is_space::imbue_stream(ss_in);

    std::atomic<bool> cancel = false;
    request_manager rm(&cancel, request_manager::async_policy::SYNC);

    server_mock dummy_server(GetParam().messages_limit);

    base_protocol_channel channel(ss_in, ss_out);
    dispatcher disp(json_channel_adapter(channel), dummy_server, rm);

    int ret = disp.run_server_loop();

    EXPECT_EQ(ret, GetParam().return_value);
    EXPECT_EQ(dummy_server.messages, GetParam().messages);
    rm.end_worker();
}

TEST(dispatcher, write_message)
{
    std::stringstream ss;
    server_mock dummy_server(1);
    std::atomic<bool> cancel = false;
    request_manager rm(&cancel, request_manager::async_policy::SYNC);
    base_protocol_channel channel(ss, ss);
    dispatcher d(json_channel_adapter(channel), dummy_server, rm);

    json message = R"("A json message")"_json;
    d.reply(message);

    EXPECT_EQ(ss.str(), "Content-Length: 16\r\n\r\n" + message.dump());

    rm.end_worker();
}
