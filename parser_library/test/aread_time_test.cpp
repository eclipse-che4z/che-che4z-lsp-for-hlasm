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

#include "aread_time.h"

using namespace hlasm_plugin::parser_library;

#include "gtest/gtest.h"

using days = std::chrono::duration<decltype(std::declval<std::chrono::seconds>().count()), std::ratio<86400>>;

TEST(aread, clockb)
{
    auto input = std::chrono::hours { 16 } + std::chrono::minutes { 45 } + std::chrono::seconds { 35 }
        + std::chrono::milliseconds { 843 };
    EXPECT_EQ(time_to_clockb(input), "06033584");
}
TEST(aread, clockd)
{
    auto input = std::chrono::hours { 16 } + std::chrono::minutes { 45 } + std::chrono::seconds { 35 }
        + std::chrono::milliseconds { 843 };

    EXPECT_EQ(time_to_clockd(input), "16453584");
}
