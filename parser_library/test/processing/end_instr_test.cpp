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

TEST(END, relocatable_symbol)
{
    std::string input(R"(
TEST CSECT     
     END TEST
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "W014" }));
}
TEST(END, no_operands)
{
    std::string input(R"(
   END ,
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
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E010" }));
}
TEST(END, absolute_symbol_false)
{
    std::string input(R"( 
UNDEF EQU 12  
     END UNDEF
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E032" }));
}
TEST(END, no_operand)
{
    std::string input(R"(   
     END 
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, two_operands_true)
{
    std::string input(R"( 
NAME     CSECT
AREA     DS              50F
ENTRYPT  BALR            2,0
     END ENTRYPT,(MYCOMPILER,0101,00273)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, two_operands_with_operand_identifier_false)
{
    std::string input(R"( 
NAME     CSECT
AREA     DS              50F
ENTRYPT  BALR            2,0
     END ENTRYPT,SDS(MYCOMPILER,0101,00273)
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A137" }));
}
TEST(END, three_operands_emptyfalse)
{
    std::string input(R"( 
NAME     CSECT
AREA     DS              50F
ENTRYPT  BALR            2,0
     END ENTRYPT,SDS(MYCOMPILER,0101,00273),
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A012" }));
}
TEST(END, three_operands_false)
{
    std::string input(R"( 
NAME     CSECT
AREA     DS              50F
ENTRYPT  BALR            2,0
     END ENTRYPT,SDS(MYCOMPILER,0101,00273),TEST
)");
    analyzer a(input);
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E010" }));
}
