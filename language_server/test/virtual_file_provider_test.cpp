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

#include "gmock/gmock.h"
#include "json_channel.h"

#include "protocol.h"
#include "virtual_file_provider.h"
#include "ws_mngr_mock.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;
using namespace hlasm_plugin::language_server::test;
using namespace ::testing;

struct json_sink_mock : public json_sink
{
    MOCK_METHOD(void, write, (const nlohmann::json&), (override));
    void write(nlohmann::json&& j) override { write(j); }
};

TEST(virtual_file_provider, predicate)
{
    ws_mngr_mock ws_mngr;
    json_sink_mock sink;

    virtual_file_provider vfp(ws_mngr, sink);
    auto pred = vfp.get_filtering_predicate();

    EXPECT_TRUE(pred(nlohmann::json { { "method", "get_virtual_file_content" } }));
    EXPECT_FALSE(pred(nlohmann::json { { "method", "get_virtual_file_content1" } }));
    EXPECT_FALSE(pred(nlohmann::json { { "method", "get_virtual_file_conten" } }));
    EXPECT_FALSE(pred(nlohmann::json { { "nested", { "method", "get_virtual_file_content" } } }));
}

TEST(virtual_file_provider, file_missing)
{
    ws_mngr_mock ws_mngr;
    json_sink_mock sink;

    virtual_file_provider vfp(ws_mngr, sink);

    ON_CALL(ws_mngr, get_virtual_file_content(0)).WillByDefault([](auto) { return continuous_sequence<char>(); });

    nlohmann::json result;

    EXPECT_CALL(sink, write).WillOnce(SaveArg<0>(&result));

    vfp.write(nlohmann::json {
        { "id", "my_id" },
        { "method", "get_virtual_file_content" },
        {
            "params",
            {
                { "id", 0 },
            },
        },
    });

    EXPECT_EQ(result.at("id"), "my_id");
    EXPECT_EQ(result.count("result"), 0);
    ASSERT_EQ(result.count("error"), 1);
}

TEST(virtual_file_provider, file_present)
{
    ws_mngr_mock ws_mngr;
    json_sink_mock sink;

    virtual_file_provider vfp(ws_mngr, sink);

    ON_CALL(ws_mngr, get_virtual_file_content(1)).WillByDefault([](auto) {
        return make_continuous_sequence(std::string("test"));
    });

    nlohmann::json result;

    EXPECT_CALL(sink, write).WillOnce(SaveArg<0>(&result));

    vfp.write(nlohmann::json {
        { "id", "my_id" },
        { "method", "get_virtual_file_content" },
        {
            "params",
            {
                { "id", 1 },
            },
        },
    });

    EXPECT_EQ(result.at("id"), "my_id");
    EXPECT_EQ(result.count("error"), 0);
    ASSERT_EQ(result.count("result"), 1);
    ASSERT_EQ(result.at("result").count("content"), 1);
    EXPECT_EQ(result.at("result").at("content"), "test");
}
