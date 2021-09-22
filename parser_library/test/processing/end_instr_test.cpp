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

TEST(END, relocatable_symbol) {
    std::string input(R"(

)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, relocatable_expression)
{
    std::string input(R"(
TEST CSECT     
     END TEST+8
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, external_symbol)
{
    std::string input(R"(
 CSECT     
     EXTRN TEST
     END TEST  
)");
    analyzer a(input);
    a.analyze();

    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, only_first_end_processed)
{
    std::string input(R"(
   END 
   END UNDEF
 )");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, undefined_symbol)
{
    std::string input(R"(   
     END UNDEF
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E032" }));
   
}

