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

TEST(timestamp, compare) { EXPECT_LT(timestamp(2000, 1, 2), timestamp(2000, 1, 2, 3, 4, 5, 6)); }

TEST(timestamp, now) { EXPECT_GT(timestamp::now(), timestamp(2000, 1, 1)); }

TEST(timestamp, components)
{
    auto t = timestamp::now().value();
    EXPECT_EQ(t, timestamp(t.year(), t.month(), t.day(), t.hour(), t.minute(), t.second(), t.microsecond()));
}

TEST(timestamp, to_string) { EXPECT_EQ(timestamp(1, 2, 3, 4, 5, 6, 7).to_string(), "0001-02-03 04:05:06.000007"); }
