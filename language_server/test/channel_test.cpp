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

#include <optional>
#include <sstream>

#include "gmock/gmock.h"
#include "json_channel_mock.h"
#include "json_queue_channel.h"

#include "base_protocol_channel.h"
#include "dap/dap_message_wrappers.h"
#include "message_router.h"
#include "nlohmann/json.hpp"
#include "stream_helper.h"

using namespace hlasm_plugin::language_server;

namespace {
struct channel_param
{
    std::string lsp_message;
    std::vector<nlohmann::json> jsons;

    static channel_param from_jsons(std::initializer_list<nlohmann::json> js)
    {
        channel_param result;
        for (const auto& j : js)
        {
            result.jsons.push_back(j);
            std::string j_string = j.dump();
            result.lsp_message += "Content-Length: ";
            result.lsp_message += std::to_string(j_string.size());
            result.lsp_message += "\r\n\r\n";
            result.lsp_message += j_string;
        }
        return result;
    }
};

class channel_fixture : public ::testing::TestWithParam<channel_param>
{};

class channel_bad_fixture : public ::testing::TestWithParam<std::string>
{};
} // namespace

INSTANTIATE_TEST_SUITE_P(channel_data,
    channel_fixture,
    ::testing::Values(channel_param::from_jsons({ "null"_json }),
        channel_param::from_jsons({ "1"_json }),
        channel_param::from_jsons({ "true"_json }),
        channel_param::from_jsons({ "{}"_json }),
        channel_param::from_jsons({ "[]"_json }),
        channel_param::from_jsons({ R"({"a":"b"})"_json }),
        channel_param::from_jsons({ R"({"a":"b"})"_json, R"({"b":"a"})"_json })));

TEST_P(channel_fixture, from_strings)
{
    std::stringstream ss_i(GetParam().lsp_message);
    std::stringstream ss_o;
    imbue_stream_newline_is_space(ss_i);
    base_protocol_channel ch(ss_i, ss_o);

    for (const auto& msg_e : GetParam().jsons)
    {
        auto msg = ch.read();
        ASSERT_TRUE(msg.has_value());
        EXPECT_EQ(msg.value(), msg_e);
    }
    ASSERT_FALSE(ch.read().has_value());
}

TEST_P(channel_fixture, to_strings)
{
    std::stringstream ss_i;
    std::stringstream ss_o;
    base_protocol_channel ch(ss_i, ss_o);

    for (const auto& msg_e : GetParam().jsons)
    {
        ch.write(msg_e);
    }
    EXPECT_EQ(ss_o.str(), GetParam().lsp_message);
}

INSTANTIATE_TEST_SUITE_P(channel_bad_data,
    channel_bad_fixture,
    ::testing::Values(R"()",
        R"(Content-Length: )",
        R"(Content-Length: 0
)",
        R"(Content-Length: 0

)",
        R"(Content-Length: 1

a)",
        R"(Content-Length: 1234567890

)"));

namespace {
std::string replace_lf_with_crlf(const std::string& s)
{
    std::string result;
    for (auto c : s)
    {
        if (c == '\n')
            result += '\r';
        result += c;
    }
    return result;
}
} // namespace

TEST_P(channel_bad_fixture, from_strings)
{
    std::string input = replace_lf_with_crlf(GetParam());
    std::stringstream ss_i(input);
    std::stringstream ss_o;
    imbue_stream_newline_is_space(ss_i);
    base_protocol_channel ch(ss_i, ss_o);

    ASSERT_FALSE(ch.read().has_value());
}

TEST(channel, adapter_source_sink)
{
    using namespace ::testing;
    mock_json_sink sink;
    mock_json_source source;

    json_channel_adapter a(source, sink);
    const nlohmann::json value = "[1, 2, 3]"_json;

    nlohmann::json lref;
    nlohmann::json rref;

    {
        InSequence sequence;
        EXPECT_CALL(source, read()).WillOnce(Return(std::optional(value)));
        EXPECT_CALL(sink, write(_)).WillOnce(SaveArg<0>(&lref));
        EXPECT_CALL(sink, write_rvr(_)).WillOnce(SaveArg<0>(&rref));
    }

    auto read_res = a.read();
    a.write(value);
    a.write(nlohmann::json(value));

    ASSERT_TRUE(read_res.has_value());
    EXPECT_EQ(read_res.value(), value);
    EXPECT_EQ(lref, value);
    EXPECT_EQ(rref, value);
}

TEST(channel, adapter_channel)
{
    using namespace ::testing;
    mock_json_channel channel;

    json_channel_adapter a(channel);
    nlohmann::json value = "[1, 2, 3]"_json;

    nlohmann::json lref;
    nlohmann::json rref;

    {
        InSequence sequence;
        EXPECT_CALL(channel, read()).WillOnce(Return(std::optional(value)));
        EXPECT_CALL(channel, write(_)).WillOnce(SaveArg<0>(&lref));
        EXPECT_CALL(channel, write_rvr(_)).WillOnce(SaveArg<0>(&rref));
    }

    auto read_res = a.read();
    a.write(value);
    a.write(nlohmann::json(value));

    ASSERT_TRUE(read_res.has_value());
    EXPECT_EQ(read_res.value(), value);
    EXPECT_EQ(lref, value);
    EXPECT_EQ(rref, value);
}

TEST(channel, dap_wrap)
{
    using namespace ::testing;
    mock_json_sink sink;
    dap::message_wrapper wrap(sink, 0);

    nlohmann::json value = "[1, 2, 3]"_json;
    auto wrapped_value =
        nlohmann::json { { "jsonrpc", "2.0" }, { "method", "hlasm/dap_tunnel/0" }, { "params", value } };

    nlohmann::json lref;
    nlohmann::json rref;

    {
        InSequence sequence;
        EXPECT_CALL(sink, write_rvr(_)).WillOnce(SaveArg<0>(&lref));
        EXPECT_CALL(sink, write_rvr(_)).WillOnce(SaveArg<0>(&rref));
    }

    wrap.write(value);
    wrap.write(nlohmann::json(value));

    EXPECT_EQ(lref, wrapped_value);
    EXPECT_EQ(rref, wrapped_value);
}

TEST(channel, dap_unwrap)
{
    using namespace ::testing;
    mock_json_source source;
    dap::message_unwrapper unwrap(source);

    nlohmann::json value = "[1, 2, 3]"_json;
    auto wrapped_value = nlohmann::json { { "jsonrpc", "2.0" }, { "method", "hlasm/dap_tunnel" }, { "params", value } };

    EXPECT_CALL(source, read()).WillOnce(Return(wrapped_value));

    auto read_value = unwrap.read();

    ASSERT_TRUE(read_value.has_value());
    EXPECT_EQ(value, read_value.value());
}

TEST(channel, blocking_queue)
{
    json_queue_channel q;
    auto ljson = "1"_json;
    auto rjson = "2"_json;
    q.write(ljson);
    q.write(nlohmann::json(rjson));

    auto lres = q.read();
    auto rres = q.read();

    ASSERT_TRUE(lres.has_value());
    ASSERT_TRUE(rres.has_value());

    EXPECT_EQ(lres.value(), ljson);
    EXPECT_EQ(rres.value(), rjson);
}
