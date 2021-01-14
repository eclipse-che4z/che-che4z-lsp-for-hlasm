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

#include <chrono>
#include <sstream>

#include "gmock/gmock.h"

#include "request_manager.h"
#include "server.h"


using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace std::chrono_literals;

class server_mock_rm : public server
{
public:
    server_mock_rm(std::atomic<bool>* cancel)
        : server(ws_mngr_)
        , cancel_(cancel)
    {}
    void message_received(const json&) override
    {
        ++messages_received;
        for (size_t i = 0; i < 50; ++i)
        {
            if (*cancel_)
                return;
            std::this_thread::sleep_for(100ms);
        }
    }

    virtual void request(const json&, const std::string&, const json&, method) override {}
    virtual void respond(const json&, const std::string&, const json&) override {}
    virtual void notify(const std::string&, const json&) override {}
    virtual void respond_error(const json&, const std::string&, int, const std::string&, const json&) override {}

    int messages_received = 0;

private:
    parser_library::workspace_manager ws_mngr_;
    std::atomic<bool>* cancel_;
};

TEST(request_manager, finish_requests)
{
    std::atomic<bool> cancel = false;
    request_manager rm(&cancel);
    server_mock_rm s(&cancel);
    server_mock_rm s2(&cancel);

    rm.add_request(&s, "0"_json);
    rm.add_request(&s, "0"_json);
    rm.add_request(&s, "0"_json);

    std::this_thread::sleep_for(100ms);

    rm.finish_server_requests(&s2);
    EXPECT_TRUE(rm.is_running());

    rm.finish_server_requests(&s);

    EXPECT_FALSE(rm.is_running());
    EXPECT_EQ(s.messages_received, 3);

    rm.finish_server_requests(&s);

    EXPECT_FALSE(rm.is_running());
    EXPECT_EQ(s.messages_received, 3);

    rm.end_worker();
}
