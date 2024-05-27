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
#include "../mock_parse_lib_provider.h"
TEST(END, relocatable_symbol)
{
    std::string input(R"(
TEST CSECT     
     END TEST
)");
    analyzer a(input);
    a.analyze();

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

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, multiple_ends)
{
    std::string input(R"(
   END 
   END UNDEF
 )");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}
TEST(END, no_operands)
{
    std::string input(R"(
   END ,
 )");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, undefined_symbol)
{
    std::string input(R"(   
     END UNDEF
)");
    analyzer a(input);
    a.analyze();

    ASSERT_TRUE(matches_message_codes(a.diags(), { "E010" }));
    EXPECT_TRUE(a.diags()[0].message.ends_with(": UNDEF"));
}
TEST(END, absolute_symbol_false)
{
    std::string input(R"( 
UNDEF EQU 12  
     END UNDEF
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E032" }));
}
TEST(END, no_operand)
{
    std::string input(R"(   
     END 
)");
    analyzer a(input);
    a.analyze();

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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A137" }));
}
TEST(END, three_operands_empty_false)
{
    std::string input(R"( 
NAME     CSECT
AREA     DS              50F
ENTRYPT  BALR            2,0
     END ENTRYPT,SDS(MYCOMPILER,0101,00273),
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A012" }));
}

TEST(END, strange_operands)
{
    std::string input(R"(
     END ,(MY-3)15/A(,01)1,0*2@3)
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, label_sequenceSymbol_true)
{
    std::string input(R"( 
.NAME  END 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}
TEST(END, label_sequenceSymbol_false)
{
    std::string input(R"( 
NAME  END 
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A249" }));
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E010" }));
}

TEST(END, end_called_from_macro_correct)
{
    std::string input(R"( 
         MACRO                                               
     MAC                                                 
     END   TEST                                   
     MEND                                                
TEST CSECT                                
     MAC
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(END, end_called_from_macro_with_statements_after_end)
{
    std::string input(R"( 
         MACRO                                               
     MAC                                                 
     END   TEST                                         
     MEND                                                
TEST CSECT                                                                                 
     MAC  
     LR 1,2     
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}

TEST(END, end_called_from_macro_with_lookahead_symbols_found)
{
    std::string input(R"( 
         MACRO                                               
     MAC                                                 
     END   TEST                                          
     MEND                                                
TEST CSECT                                                   
     AIF   (L'X LT 0).X                                 
     MAC
     BR    14                                            
.X   ANOP  ,                                            
X    DS    C                                                          
     BR    0                                             
     END                                                 
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}

TEST(END, end_called_from_macro_inside_copy_book_one_with_warning_after_end)
{
    std::string copybook_content = R"(
   MACRO
  M
  LR 1,2
  END 
 MEND       
)";
    std::string input = R"(
         COPY COPYBOOK_TWO
         M
         LR 1,2
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK_TWO", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}
TEST(END, end_called_from_copybook)
{
    std::string copybook_content = R"(

  LR 1,2
  END
  UNDEF       
)";
    std::string input = R"(
         COPY COPYBOOK_TWO
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK_TWO", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, end_called_from_copybook_with_warning_after_EndStatement)
{
    std::string copybook_content = R"(

  LR 1,2
  END
         
)";
    std::string input = R"(
         COPY COPYBOOK_TWO
         UNDEF
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK_TWO", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}

TEST(END, end_called_from_macro_inside_copybook_two)
{
    std::string copybook_content = R"(
  MACRO
  M
  LR 1,2
  END 
  UNDEF
  MEND
)";
    std::string input = R"(
        COPY COPYBOOK
        M
        
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, end_called_from_nested_macro)
{
    std::string input = R"(
    MACRO
    OUTER_MACRO                               
    INNER_MACRO 
    MEND   
    MACRO
    INNER_MACRO                               
    END
    UNDEF
    MEND
    OUTER_MACRO
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, end_called_from_nested_macro_with_warning_issued)
{
    std::string input = R"(
    MACRO
    OUTER_MACRO                               
    INNER_MACRO 
    MEND   
    MACRO
    INNER_MACRO                               
    END
    UNDEF
    MEND
    OUTER_MACRO
    UNDEF
)";
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}
TEST(END, end_called_from_copy_book_inside_macro_with_warning)
{
    std::string input = R"(
    MACRO
      MAC
    COPY COPYBOOK
      MEND
     MAC
     END    
)";
    std::string copybook_content = R"(
  LR 1,2
  END     
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W015" }));
}
TEST(END, end_called_from_copy_book_inside_macro)
{
    std::string input = R"(
    MACRO
    MAC
    COPY COPYBOOK
    NOT_PROCESSED_LINE
    MEND
    MAC    
)";
    std::string copybook_content = R"(
  LR 1,2
  END     
)";
    mock_parse_lib_provider lib_provider { { "COPYBOOK", copybook_content } };
    analyzer a(input, analyzer_options { &lib_provider });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
TEST(END, stops_lookahead)
{
    std::string input = R"(
         AIF (L'X LT 0).X
.X       END
.X       ANOP
)";
    analyzer a(input);

    a.analyze();

    EXPECT_EQ(0, std::ranges::count(a.diags(), "E045", &diagnostic::code));
}
