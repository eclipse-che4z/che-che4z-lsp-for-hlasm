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

    ASSERT_EQ(a.diags().size(), (size_t)1);

    EXPECT_EQ(a.diags()[0].diag_range.start.line, (size_t)2);
    EXPECT_EQ(a.diags()[0].related.size(), (size_t)2);
    EXPECT_EQ(a.diags()[0].related[0].location.rang.start.line, (size_t)8);
    EXPECT_EQ(a.diags()[0].related[1].location.rang.start.line, (size_t)13);
}
