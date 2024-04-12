/*
 * Copyright (c) 2019 Broadcom.
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

#include "common_testing.h"

// test for
// correctly outputed structure of nested diagnostic

TEST(diagnosable_ctx, one_file_diag)
{
    std::string input =
        R"( MACRO
 M2
 lr 1,
 MEND

 MACRO
 M1
 lr 1,1
 M2
 anop
 mend

 lr 1,1
 M1
)";
    analyzer a(input);
    a.analyze();

    auto diags = a.diags();
    ASSERT_EQ(diags.size(), (size_t)1);

    const auto& d = diags.front();
    EXPECT_EQ(d.diag_range.start.line, (size_t)2);
    EXPECT_EQ(d.related.size(), (size_t)2);
    EXPECT_EQ(d.related[0].location.rang.start.line, (size_t)8);
    EXPECT_EQ(d.related[1].location.rang.start.line, (size_t)13);
}
