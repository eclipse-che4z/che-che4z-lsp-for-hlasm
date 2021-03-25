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

#include <sstream>
#include <thread>
#include <utility>

#include "gmock/gmock.h"

#include "blocking_queue.h"

using namespace hlasm_plugin::language_server;

namespace {
struct queue_data
{
    static std::pair<int, int>& get_call_counts()
    {
        static std::pair<int, int> call_counts = {};
        return call_counts;
    }

    int data;
    queue_data(int i)
        : data(i)
    {}
    queue_data(const queue_data& qd) noexcept
        : data(qd.data)
    {
        get_call_counts().first++;
    }
    queue_data(queue_data&& qd) noexcept
        : data(qd.data)
    {
        get_call_counts().second++;
    }
};
} // namespace

TEST(blocking_queue, simple_io)
{
    blocking_queue<queue_data> queue;
    constexpr const int limit = 1023;
    for (int i = 0; i < limit; ++i)
    {
        if (i % 2)
            queue.push(queue_data(i));
        else
        {
            queue_data qe(i);
            queue.push(qe);
        }
    }

    auto constructor_counts = queue_data::get_call_counts();

    EXPECT_EQ(constructor_counts.first, limit - limit / 2);
    EXPECT_EQ(constructor_counts.second, limit / 2);

    for (int i = 0; i < limit; ++i)
    {
        auto data = queue.pop();
        ASSERT_TRUE(data.has_value());
        EXPECT_EQ(data.value().data, i);
    }
}

TEST(blocking_queue, terminate)
{
    blocking_queue<int> queue;
    queue.terminate();
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(blocking_queue, multithreaded)
{
    constexpr const int message_limit = 1024 * 1024;
    blocking_queue<int> queue;
    std::thread reader([&]() {
        int counter = 0;
        while (counter != message_limit)
        {
            auto msg = queue.pop();
            ASSERT_TRUE(msg.has_value());
            EXPECT_EQ(msg.value(), counter);
            ++counter;
        }
    });
    std::thread writer([&]() {
        for (int i = 0; i < message_limit; ++i)
            queue.push(i);
    });
    writer.join();
    reader.join();
}
