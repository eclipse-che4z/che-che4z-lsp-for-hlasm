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
#include "context/literal_pool.h"

// test for
// handling of literals

TEST(literals, duplicate_when_loctr_references)
{
    std::string input = R"(
      MACRO
      MAC
      LARL 0,=A(*)
      MEND
      MAC
      MAC
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.hlasm_ctx().ord_ctx.literals().get_pending_count(), 2);
}

TEST(literals, unique_when_no_loctr_references)
{
    std::string input = R"(
      MACRO
      MAC
      LARL 0,=A(0)
      MEND
      MAC
      MAC
)";
    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.hlasm_ctx().ord_ctx.literals().get_pending_count(), 1);
}

TEST(literals, no_nested_literals)
{
    std::string input = R"(
    LARL 0,=A(=A(0))
    DC   A(=A(0))
)";
    analyzer a(input);
    a.analyze();
    a.collect_diags();

    auto& d = a.diags();
    EXPECT_EQ(std::count_if(d.begin(), d.end(), [](const auto& m) { return m.code == "S0013"; }), 2);
}
