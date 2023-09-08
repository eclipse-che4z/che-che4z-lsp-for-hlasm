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

#include "utils/platform.h"

using namespace hlasm_plugin::utils::platform;

TEST(platform, home)
{
    auto homedir = home();

    if (is_web())
        EXPECT_EQ(homedir.size(), 0);
    else
        EXPECT_GT(homedir.size(), 0);
}
