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

#include "gtest/gtest.h"

#include "../common_testing.h"

using namespace hlasm_plugin::parser_library;

TEST(instruction_label, empty_concatenation)
{
    std::string input(R"(
&A    SETC  ''
&A.   DS    A
&A    SETC  ' '
&A.   DS    A
&A.&A DS    A
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    const auto& diags = a.diags();

    EXPECT_EQ(diags.size(), 0);
}

TEST(instruction_label, invalid_concatenation)
{
    std::string input(R"(
&A    SETC  ' L'
&A.   DS    A
&A    SETC  ' '
&A.L  DS    A
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    const auto& diags = a.diags();

    EXPECT_EQ(diags.size(), 2);
}
