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

using namespace hlasm_plugin::utils::resource;
TEST(deferred_statement, split_var)
{
    std::string input = R"(
   MACRO
   MAC &AA
   &AA C'                                                            &AX
               A'
   MEND

   MAC DC
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());

    auto aa1 = a.context().lsp_ctx->hover(resource_location(""), { 3, 71 });
    auto aa2 = a.context().lsp_ctx->hover(resource_location(""), { 4, 16 });
    EXPECT_EQ(aa1, "MACRO parameter");
    EXPECT_EQ(aa2, "MACRO parameter");
}

TEST(deferred_statement, nested_macro_def_with_continuation)
{
    std::string input = R"(
 MACRO
 MAC

 MACRO
 MAC2                                                                  X
               &OP=,                                                   X
               &OP2=

 MEND

 MEND

 MAC

 MAC2 OP=A,OP2=B
)";
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}