/*
 * Copyright (c) 2021 Broadcom.
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

#include <array>
#include <optional>
#include <sstream>

#include "gmock/gmock.h"
#include "json.hpp"

#include "message_router.h"

using namespace hlasm_plugin::language_server;

namespace {
struct mock_json_sink : public json_sink
{
    MOCK_METHOD1(write, void(const nlohmann::json&));
    MOCK_METHOD1(write_rvr, void(nlohmann::json&&));
    void write(nlohmann::json&& j) override { write_rvr(std::move(j)); }
};
} // namespace

TEST(message_router, with_default)
{
    using namespace ::testing;

    mock_json_sink route1;
    mock_json_sink default_route;

    message_router router(&default_route);
    router.register_route([](const nlohmann::json& j) { return j.get<std::string>() == "router1"; }, route1);

    auto router1_msg = R"("router1")"_json;
    auto default_msg = R"("default")"_json;

    std::array<nlohmann::json, 4> results;
    {
        InSequence sequence;

        EXPECT_CALL(route1, write(_)).WillOnce(SaveArg<0>(results.data() + 0));
        EXPECT_CALL(default_route, write(_)).WillOnce(SaveArg<0>(results.data() + 1));
        EXPECT_CALL(route1, write_rvr(_)).WillOnce(SaveArg<0>(results.data() + 2));
        EXPECT_CALL(default_route, write_rvr(_)).WillOnce(SaveArg<0>(results.data() + 3));
    }
    router.write(router1_msg);
    router.write(default_msg);
    router.write(nlohmann::json(router1_msg));
    router.write(nlohmann::json(default_msg));

    EXPECT_EQ(results[0], router1_msg);
    EXPECT_EQ(results[1], default_msg);
    EXPECT_EQ(results[2], router1_msg);
    EXPECT_EQ(results[3], default_msg);
}

TEST(message_router, without_default)
{
    using namespace ::testing;

    mock_json_sink route1;
    mock_json_sink default_route;

    message_router router;
    router.register_route([](const nlohmann::json& j) { return j.get<std::string>() == "router1"; }, route1);
    router.register_route([](const nlohmann::json&) { return true; }, default_route);

    auto router1_msg = R"("router1")"_json;
    auto default_msg = R"("default")"_json;

    std::array<nlohmann::json, 4> results;
    {
        InSequence sequence;

        EXPECT_CALL(route1, write(_)).WillOnce(SaveArg<0>(results.data() + 0));
        EXPECT_CALL(default_route, write(_)).WillOnce(SaveArg<0>(results.data() + 1));
        EXPECT_CALL(route1, write_rvr(_)).WillOnce(SaveArg<0>(results.data() + 2));
        EXPECT_CALL(default_route, write_rvr(_)).WillOnce(SaveArg<0>(results.data() + 3));
    }
    router.write(router1_msg);
    router.write(default_msg);
    router.write(nlohmann::json(router1_msg));
    router.write(nlohmann::json(default_msg));

    EXPECT_EQ(results[0], router1_msg);
    EXPECT_EQ(results[1], default_msg);
    EXPECT_EQ(results[2], router1_msg);
    EXPECT_EQ(results[3], default_msg);
}
