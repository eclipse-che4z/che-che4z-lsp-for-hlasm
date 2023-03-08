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

TEST(workspace_manager_response, invalidate)
{
    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    MockFunction<void()> invalidator;
    p.set_invalidation_callback(invalidator.AsStdFunction());

    EXPECT_CALL(invalidator, Call());

    EXPECT_TRUE(p.valid());

    p.invalidate();

    EXPECT_FALSE(p.valid());
}

TEST(workspace_manager_response, change_invalidator)
{
    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    MockFunction<void()> invalidator;
    p.set_invalidation_callback(invalidator.AsStdFunction());
    MockFunction<void()> invalidator2;
    p.set_invalidation_callback(invalidator2.AsStdFunction());

    EXPECT_TRUE(p.valid());
    EXPECT_CALL(invalidator, Call()).Times(0);
    EXPECT_CALL(invalidator2, Call());

    p.invalidate();

    EXPECT_FALSE(p.valid());
}

thread_local int simple_invalidator_counter = 0;

void simple_invalidator() { ++simple_invalidator_counter; }

TEST(workspace_manager_response, simple_invalidator)
{
    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    p.set_invalidation_callback(simple_invalidator);

    int baseline = simple_invalidator_counter;

    p.invalidate();

    EXPECT_EQ(simple_invalidator_counter, baseline + 1);
}

TEST(workspace_manager_response, invalidator_deleter)
{
    int deleter_called = 0;

    {
        auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

        struct invalidator_t
        {
            int* d;
            void operator()() const {}

            invalidator_t(int& d)
                : d(&d)
            {}
            invalidator_t(invalidator_t&& o) noexcept
                : d(std::exchange(o.d, nullptr))
            {}
            ~invalidator_t()
            {
                if (d)
                    ++*d;
            }
        };
        p.set_invalidation_callback(invalidator_t(deleter_called));
        EXPECT_EQ(deleter_called, 0);
        p.set_invalidation_callback(invalidator_t(deleter_called));
        EXPECT_EQ(deleter_called, 1);
    }

    EXPECT_EQ(deleter_called, 2);
}

TEST(workspace_manager_response, invalidator_remove)
{
    int deleter_called = 0;

    auto [p, _impl] = make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<int>>);

    struct invalidator_t
    {
        int* d;
        void operator()() const {}

        invalidator_t(int& d)
            : d(&d)
        {}
        invalidator_t(invalidator_t&& o) noexcept
            : d(std::exchange(o.d, nullptr))
        {}
        ~invalidator_t()
        {
            if (d)
                ++*d;
        }
    };
    p.set_invalidation_callback(invalidator_t(deleter_called));

    EXPECT_EQ(deleter_called, 0);
    p.remove_invalidation_handler();
    EXPECT_EQ(deleter_called, 1);
}
