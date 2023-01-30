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

#include "nlohmann/json.hpp"
#include "request_manager.h"
#include "server.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace std::chrono_literals;

class server_mock_rm : public server
{
public:
    server_mock_rm(parser_library::workspace_manager& ws_mngr, std::atomic<bool>* cancel)
        : server(ws_mngr)
        , cancel_(cancel)
    {}
    void message_received(const nlohmann::json&) override
    {
        ++messages_received;
        for (size_t i = 0; i < 50; ++i)
        {
            if (*cancel_)
                return;
            std::this_thread::sleep_for(100ms);
        }
    }

    void request(const std::string&, const nlohmann::json&, method) override {}
    void respond(const nlohmann::json&, const std::string&, const nlohmann::json&) override {}
    void notify(const std::string&, const nlohmann::json&) override {}
    void respond_error(
        const nlohmann::json&, const std::string&, int, const std::string&, const nlohmann::json&) override
    {}

    int messages_received = 0;

private:
    std::atomic<bool>* cancel_;
};

TEST(request_manager, finish_requests)
{
    parser_library::workspace_manager ws_mngr;
    std::atomic<bool> cancel = false;
    request_manager rm(&cancel);
    server_mock_rm s(ws_mngr, &cancel);
    server_mock_rm s2(ws_mngr, &cancel);

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
