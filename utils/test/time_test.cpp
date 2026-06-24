/*
 * Copyright (c) 2022 Broadcom.
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

#include "utils/time.h"

using namespace hlasm_plugin::utils;

TEST(timestamp, compare)
{
    const timestamp t1 { 2000, 1, 2 };
    const timestamp t2 { 2000, 1, 2, 3, 4, 5, 6 };
    EXPECT_LT(t1, t2);
}

TEST(timestamp, now)
{
    const timestamp year2000 { 2000, 1, 1 };
    EXPECT_GT(timestamp::now(), year2000);
}

TEST(timestamp, to_string)
{
    const timestamp t { 1, 2, 3, 4, 5, 6, 7 };
    EXPECT_EQ(t.to_string(), "0001-02-03 04:05:06.000007");
}
