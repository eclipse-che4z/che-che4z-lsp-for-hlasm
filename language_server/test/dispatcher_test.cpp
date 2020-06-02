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

#include "dispatcher.h"
#include "stream_helper.h"


using namespace hlasm_plugin::language_server;

class server_mock : public server
{
    hlasm_plugin::parser_library::workspace_manager ws;
    int counter = 0;
    int num_messages;

public:
    server_mock(int max_messages)
        : server(ws)
        , num_messages(max_messages)
    {}

    std::vector<json> messages;

    virtual void respond(const json&, const std::string&, const json&) override {}
    virtual void notify(const std::string&, const json&) override {}
    virtual void respond_error(const json&,
        const std::string&,
        int,
        const std::string&,
        const json&) override
    {}
    virtual void message_received(const json& message) override
    {
        ++counter;
        if (counter == 1)
        {
            shutdown_request_received_ = true;
            exit_notification_received_ = true;
        }
        messages.push_back(message);
    }
};

TEST(dispatcher, run_server_loop)
{
    json message1 = "\"A json message 1\""_json;
    json message2 = R"({"second":"json message"})"_json;
    std::stringstream ss_in;
    std::stringstream ss_out;
    ss_in << "Content-Length: 18\r\n";
    ss_in << "Should be ignored\r\n";
    ss_in << "\r\n";
    ss_in << message1.dump();
    ss_in << "Content-Length: " << message2.dump().size() << "\r\n";
    ss_in << "\r\n";
    ss_in << message2.dump();
    newline_is_space::imbue_stream(ss_in);

    std::atomic<bool> cancel;
    request_manager rm(&cancel);

    server_mock dummy_server(2);


    dispatcher disp(ss_in, ss_out, dummy_server, rm);

    disp.run_server_loop();

    ASSERT_EQ(dummy_server.messages.size(), 2);
    EXPECT_EQ(dummy_server.messages[0], message1);
    EXPECT_EQ(dummy_server.messages[1], message2);
    rm.end_worker();
}

TEST(dispatcher, unexpected_eof)
{
    std::stringstream ss_in;
    std::stringstream ss_out;
    ss_in << "Content-Length: 30\r\n";
    ss_in << "Should be ignored\r\n";
    ss_in << "\r\n";
    ss_in << "\"A json message 1\"";
    newline_is_space::imbue_stream(ss_in);

    std::atomic<bool> cancel;
    request_manager rm(&cancel);

    server_mock dummy_server(1);


    dispatcher disp(ss_in, ss_out, dummy_server, rm);

    disp.run_server_loop();

    EXPECT_EQ(dummy_server.messages.size(), 0U);
    rm.end_worker();
}

TEST(dispatcher, unexpected_content_length)
{
    std::stringstream ss_in;
    std::stringstream ss_out;
    ss_in << "Content-Length: 3000000000000000000\r\n";
    ss_in << "Should be ignored\r\n";
    ss_in << "\r\n";
    ss_in << "\"A json message 1\"";
    newline_is_space::imbue_stream(ss_in);

    std::atomic<bool> cancel;
    request_manager rm(&cancel);

    server_mock dummy_server(1);


    dispatcher disp(ss_in, ss_out, dummy_server, rm);

    disp.run_server_loop();

    EXPECT_EQ(dummy_server.messages.size(), 0U);
    rm.end_worker();
}