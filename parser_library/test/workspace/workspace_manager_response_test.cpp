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

#include <thread>

#include "gtest/gtest.h"

#include "../workspace_manager_response_mock.h"
#include "workspace_manager_response.h"

using namespace hlasm_plugin::parser_library;
using namespace ::testing;

struct lifetime_mock : workspace_manager_response_mock<int>
{
    MOCK_METHOD(bool, destructor, (), ());

    ~lifetime_mock() { destructor(); }
};

TEST(workspace_manager_response, destructor_called)
{
    auto [p, impl] = make_workspace_manager_response(std::in_place_type<lifetime_mock>);

    EXPECT_CALL(*impl, destructor());
}

TEST(workspace_manager_response, copy)
{
    auto [p, impl] = make_workspace_manager_response(std::in_place_type<lifetime_mock>);

    {
        auto q = p;
    }

    EXPECT_CALL(*impl, destructor());
}

TEST(workspace_manager_response, move)
{
    auto [p, impl] = make_workspace_manager_response(std::in_place_type<lifetime_mock>);

    auto q = std::move(p);

    EXPECT_CALL(*impl, destructor());
}

TEST(workspace_manager_response, copy_assign)
{
    workspace_manager_response<int> q;
    lifetime_mock* impl;

    {
        auto [p, p_impl] = make_workspace_manager_response(std::in_place_type<lifetime_mock>);

        impl = p_impl;
        q = p;
    }

    EXPECT_CALL(*impl, destructor());
}

TEST(workspace_manager_response, move_assign)
{
    workspace_manager_response<int> q;
    lifetime_mock* impl;

    {
        auto [p, p_impl] = make_workspace_manager_response(std::in_place_type<lifetime_mock>);

        impl = p_impl;
        q = std::move(p);
    }

    EXPECT_CALL(*impl, destructor());
}

TEST(workspace_manager_response, provide)
{
    auto [p, impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    EXPECT_CALL(*impl, provide(5));

    p.provide(5);
}

TEST(workspace_manager_response, error)
{
    auto [p, impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    EXPECT_CALL(*impl, error(5, StrEq("Error message")));

    p.error(5, "Error message");
}

TEST(workspace_manager_response, invalidate_without_handler)
{
    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    EXPECT_TRUE(p.valid());

    EXPECT_NO_FATAL_FAILURE(p.invalidate());

    EXPECT_FALSE(p.valid());
}


TEST(workspace_manager_response, multithreaded)
{
    using namespace std::chrono_literals;

    struct test_t
    {
        int result = 0;
        void error(int, const char*) noexcept {}
        void provide(int v) noexcept { result = v; }
    };
    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<test_t>);

    EXPECT_TRUE(p.valid());
    EXPECT_FALSE(p.resolved());

    std::thread receiver([p = p, _impl = _impl]() {
        while (!p.resolved())
        {
            std::this_thread::sleep_for(10ms);
        }
        EXPECT_EQ(_impl->result, 5);
    });
    std::thread sender([p = p]() {
        std::this_thread::sleep_for(200ms);
        p.provide(5);
    });

    sender.join();
    receiver.join();

    EXPECT_NO_FATAL_FAILURE(p.invalidate());

    EXPECT_FALSE(p.valid());
    EXPECT_TRUE(p.resolved());
    EXPECT_EQ(_impl->result, 5);
}
