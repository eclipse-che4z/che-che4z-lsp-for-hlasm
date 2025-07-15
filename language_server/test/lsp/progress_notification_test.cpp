/*
 * Copyright (c) 2025 Broadcom.
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
#include "lsp/progress_notification.h"
#include "nlohmann/json.hpp"

using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::language_server::lsp;
using namespace ::testing;

TEST(progress_notification, supported)
{
    const auto init_params = R"({
    "capabilities": {
        "window": {
            "workDoneProgress": true
        }
    }
})"_json;
    EXPECT_TRUE(progress_notification::client_supports_work_done_progress(init_params));
}

TEST(progress_notification, not_supported)
{
    const auto init_params = R"({
    "capabilities": {
        "window": {
            "workDoneProgress": false
        }
    }
})"_json;
    EXPECT_FALSE(progress_notification::client_supports_work_done_progress(init_params));
}

TEST(progress_notification, missing)
{
    const auto init_params = R"({
    "capabilities": {
        "window": {
        }
    }
})"_json;
    EXPECT_FALSE(progress_notification::client_supports_work_done_progress(init_params));
}

constexpr std::string_view progress_event = "$/progress";
constexpr std::string_view progress_create = "window/workDoneProgress/create";
TEST(progress_notification, missing_start)
{
    StrictMock<response_provider_mock> mock;

    progress_notification p(mock);

    p.parsing_started({});
}

TEST(progress_notification, multiple_parsing)
{
    StrictMock<response_provider_mock> mock;

    std::function<void(const nlohmann::json&)> callback;

    auto token_0 = R"({"token":0})"_json;
    auto token_0_a = R"({"token":0,"value":{"kind":"begin","title":"Parsing","message":"a"}})"_json;
    auto token_0_b = R"({"token":0,"value":{"kind":"report","message":"b"}})"_json;
    auto token_0_end = R"({"token":0,"value":{"kind":"end"}})"_json;
    auto token_1 = R"({"token":1})"_json;
    auto token_1_a = R"({"token":1,"value":{"kind":"begin","title":"Parsing","message":"a"}})"_json;
    auto token_1_end = R"({"token":1,"value":{"kind":"end"}})"_json;

    EXPECT_CALL(mock, request(progress_create, std::move(token_0), _, _)).WillOnce(SaveArg<2>(&callback));
    EXPECT_CALL(mock, notify(progress_event, std::move(token_0_a))).Times(1);
    EXPECT_CALL(mock, notify(progress_event, std::move(token_0_b))).Times(1);
    EXPECT_CALL(mock, notify(progress_event, std::move(token_0_end))).Times(1);
    EXPECT_CALL(mock, request(progress_create, std::move(token_1), _, _)).WillOnce(SaveArg<2>(&callback));
    EXPECT_CALL(mock, notify(progress_event, std::move(token_1_a))).Times(1);
    EXPECT_CALL(mock, notify(progress_event, std::move(token_1_end))).Times(1);

    progress_notification p(mock);

    p.parsing_started("a");
    callback({});
    p.parsing_started("b");
    p.parsing_started({});

    p.parsing_started("a");
    callback({});
    p.parsing_started({});
}

TEST(progress_notification, done_before_token)
{
    StrictMock<response_provider_mock> mock;

    std::function<void(const nlohmann::json&)> callback;

    auto token_0 = R"({"token":0})"_json;
    auto token_0_end = R"({"token":0,"value":{"kind":"end"}})"_json;

    EXPECT_CALL(mock, request(progress_create, std::move(token_0), _, _)).WillOnce(SaveArg<2>(&callback));
    EXPECT_CALL(mock, notify(progress_event, std::move(token_0_end))).Times(1);

    progress_notification p(mock);

    p.parsing_started("a");
    p.parsing_started({});
    callback({});
}
