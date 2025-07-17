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

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "context/hlasm_context.h"
#include "context/variables/set_symbol.h"
#include "ebcdic_encoding.h"
#include "expressions/conditional_assembly/ca_expression.h"
#include "expressions/evaluation_context.h"
#include "library_info_transitional.h"
#include "lsp/lsp_context.h"

// tests for lookahead feature:
// forward/backward jums
// copy/macro jumps
// sequence symbol/attribute lookahead

TEST(lookahead, forward_jump_success)
{
    std::string input(
        R"( 
   AGO .A  
&new seta 1 
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    auto id = id_index("NEW");
    auto var = a.hlasm_ctx().get_var_sym(id);
    EXPECT_FALSE(var);
}

TEST(lookahead, forward_jump_to_continued)
{
    std::string input(
        R"( 
      AGO      .HERE
&bad seta 1
.HERE LR                                                               x
               1,1
&good seta 1
      LR 1,1
)");

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_FALSE(a.hlasm_ctx().get_var_sym(id_index("BAD")));
    EXPECT_TRUE(a.hlasm_ctx().get_var_sym(id_index("GOOD")));
}

TEST(lookahead, forward_jump_from_continued)
{
    std::string input(
        R"( 
      AGO                                                              x
               .HERE
&bad seta 1
.HERE LR                                                               x
               1,1
&good seta 1
      LR 1,1
)");

    analyzer a(input);
    a.analyze();
    EXPECT_EQ(a.diags().size(), (size_t)0);

    EXPECT_FALSE(a.hlasm_ctx().get_var_sym(id_index("BAD")));
    EXPECT_TRUE(a.hlasm_ctx().get_var_sym(id_index("GOOD")));
}

TEST(lookahead, forward_jump_success_valid_input)
{
    std::string input(
        R"( 
   AGO .A  
&new seta 1 
das cvx
tr9023-22
=f2 **
.A ANOP)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto id = id_index("NEW");
    auto var = a.hlasm_ctx().get_var_sym(id);
    EXPECT_FALSE(var);
}

TEST(lookahead, forward_jump_fail)
{
    std::string input(
        R"( 
   AGO .A  
&new seta 1 
.B ANOP
)");

    analyzer a(input);
    a.analyze();

    auto id = id_index("NEW");
    auto var = a.hlasm_ctx().get_var_sym(id);
    EXPECT_TRUE(var);
}

TEST(lookahead, rewinding_from_last_line)
{
    std::string input(
        R"( 
 ACTR 2 
.A ANOP
 AGO .A)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(lookahead, rewinding_from_one_from_last_line)
{
    std::string input(
        R"( 
 ACTR 2 
.A ANOP
 AGO .A
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(lookahead, forward_jump_before_comment)
{
    std::string input(
        R"( 
 AGO .A
 BAD_INSTR
*COMMENT
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(lookahead, forward_jump_before_continued_comment)
{
    std::string input(
        R"( 
 AGO .A
 BAD_INSTR
*COMMENT                                                               X IGNORED
                 COMMENT
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(lookahead, jump_to_incomplete_instruction)
{
    std::string input(
        R"(
&A       SETC  'P'
         AGO   .A
         MNOTE '123'
         AGO   .END
.A       ANO&A
.END     END
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(attribute_lookahead, lookup_triggered)
{
    std::string input("L'X");
    analyzer a(input);
    auto expr = parse_ca_expression(a);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { a.hlasm_ctx(), library_info_transitional::empty, diags };

    std::vector<context::id_index> references;
    EXPECT_TRUE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)1);

    EXPECT_EQ(diags.diags.size(), (size_t)0);
}

TEST(attribute_lookahead, nested_lookup_triggered)
{
    std::string input("L'&V1(L'&V2)");
    analyzer a(input);
    auto expr = parse_ca_expression(a);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { a.hlasm_ctx(), library_info_transitional::empty, diags };

    auto v1 = a.hlasm_ctx().create_local_variable<context::C_t>(id_index("V1"), false);
    v1->access_set_symbol<context::C_t>()->set_value("A", 1);
    auto v2 = a.hlasm_ctx().create_local_variable<context::C_t>(id_index("V2"), true);
    v2->access_set_symbol<context::C_t>()->set_value("B");

    std::vector<context::id_index> references;
    EXPECT_TRUE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)1);
    EXPECT_EQ(std::ranges::count(references, id_index("B")), 1);

    a.hlasm_ctx().ord_ctx.add_symbol_reference(id_index("B"),
        context::symbol_attributes(context::symbol_origin::EQU, 'U'_ebcdic, 1),
        library_info_transitional::empty);

    references.clear();
    EXPECT_TRUE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)1);
    EXPECT_EQ(std::ranges::count(references, id_index("A")), 1);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_not_triggered)
{
    std::string input("L'X");
    analyzer a(input);
    auto expr = parse_ca_expression(a);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { a.hlasm_ctx(), library_info_transitional::empty, diags };

    // define symbol with undefined length
    auto tmp = a.hlasm_ctx().ord_ctx.create_symbol(
        id_index("X"), symbol_value(), symbol_attributes(symbol_origin::DAT, 200), library_info_transitional::empty);
    ASSERT_TRUE(tmp);

    // although length is undefined the actual symbol is defined so no lookup should happen
    std::vector<context::id_index> references;
    EXPECT_FALSE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)0);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs)
{
    std::string input("L'X+L'Y");
    analyzer a(input);
    auto expr = parse_ca_expression(a);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { a.hlasm_ctx(), library_info_transitional::empty, diags };

    std::vector<context::id_index> references;
    EXPECT_TRUE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)2);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs_but_one_symbol)
{
    std::string input("S'X+L'X");
    analyzer a(input);
    auto expr = parse_ca_expression(a);

    diagnostic_op_consumer_container diags;
    evaluation_context eval_ctx { a.hlasm_ctx(), library_info_transitional::empty, diags };

    std::vector<context::id_index> references;
    EXPECT_TRUE(expr->get_undefined_attributed_symbols(references, eval_ctx));
    EXPECT_EQ(references.size(), (size_t)2);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, correct_attribute_refereces)
{
    std::string input(
        R"( 
&A SETC T'X
&B SETA L'X
X EQU 1,10,C'T'
&C SETA L'Y
Y EQU X+1
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "T");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "B"), 10);
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "C"), 10);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, incorrect_attribute_reference)
{
    std::string input(
        R"( 
&A SETA S'X
X EQU 1,10,C'T'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 0);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(EQU_attribute_lookahead, unresolvable_attribute_reference)
{
    std::string input(
        R"( 
&A SETA L'X
X EQU 1,Y+11,C'T'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_EQ(a.diags().size(), (size_t)1);
    EXPECT_EQ(a.diags().front().diag_range.start.line, (size_t)2);
}

TEST(EQU_attribute_lookahead, empty_operand)
{
    std::string input(
        R"( 
&A SETC T'X
X EQU 1,,C'T'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "A"), "T");

    EXPECT_TRUE(a.diags().empty());
}

TEST(EQU_attribute_lookahead, errorous_but_resolable_statement_incorrect_operand)
{
    std::string input(
        R"( 
&A SETA L'X
 AGO .A
X EQU 1,2,**&
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, errorous_but_resolable_statement_last_operand_model)
{
    std::string input(
        R"( 
&A SETA L'X
 AGO .A
X EQU 1,2,&a
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(EQU_attribute_lookahead, errorous_but_unresolable_statement_first_operand_model)
{
    std::string input(
        R"( 
&A SETA L'X
 AGO .A
X EQU &a,2
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(EQU_attribute_lookahead, errorous_but_unresolable_statement_first_operand_invalid)
{
    std::string input(
        R"( 
&A SETA L'X
 AGO .A
X EQU =**)-,2
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, lookup_to_copy)
{
    std::string input(
        R"( 
&A SETA L'X
&WAS_BEFORE SETB 1
 COPY LIB
&WAS_AFTER SETB 1
)");
    std::string LIB =
        R"( 
X EQU 1,2,C'X'
&WAS_IN SETB 1
)";

    mock_parse_lib_provider mock { { "LIB", LIB } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_BEFORE"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_IN"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_AFTER"), true);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_copy)
{
    std::string input(
        R"( 
&WAS_BEFORE SETB 1
 COPY LIB2
X EQU 1,2
&WAS_AFTER SETB 1
)");

    std::string LIB2 =
        R"( 
&A SETA L'X
&WAS_IN SETB 1
)";

    mock_parse_lib_provider mock { { "LIB2", LIB2 } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_BEFORE"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_IN"), true);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "WAS_AFTER"), true);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_macro)
{
    std::string input(
        R"(
 MACRO
 MAC
 GBLA A
&A SETA L'X
X EQU 2,3
 MEND
*
 COPY LIB3
X EQU 1,2
)");
    std::string LIB3 =
        R"( 
 MAC 
&AFTER_MAC SETB 1
)";

    mock_parse_lib_provider mock { { "LIB3", LIB3 } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    EXPECT_EQ(get_global_var_value<A_t>(a.hlasm_ctx(), "A"), 2);
    EXPECT_EQ(get_var_value<B_t>(a.hlasm_ctx(), "AFTER_MAC"), true);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, lookup_from_macro_last_line)
{
    std::string input(
        R"( macro
 GETMAIN &b=,&l=
 AIF   (T'&l NE 'O' AND T'&b NE 'O').ERR14      @L1A 
 mend
         GETMAIN   b=svc)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_from_macro_one_to_last_line)
{
    std::string input(
        R"( macro
 GETMAIN &b=,&l=
 AIF   (T'&l NE 'O' AND T'&b NE 'O').ERR14      @L1A 
 mend
         GETMAIN   b=svc
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookup_of_two_refs_evaluation)
{
    std::string input(
        R"( 
&A SETA L'X+L'Y
X EQU 1,10
Y EQU 2,11
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 21);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, machine_label_lookahead)
{
    std::string input(
        R"( 
&A SETA L'X
&B SETC T'X
X LR 1,1
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 2);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "I");

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, section_label_lookahead)
{
    std::string input(
        R"( 
&A SETA L'X
&B SETC T'X
X CSECT
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "J");

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, concat_string)
{
    std::string input(
        R"(
&C(1)    SETC 'A','B','C','D'
&D       SETC '&C(L'X)'
X        DS   F
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "D"), "D");
}

TEST(data_def_attribute_lookahead, correct_attribute_reference)
{
    std::string input(
        R"( 
&A SETA L'X
&B SETC T'X
&C SETA S'X
X DC FS24'6'       remark
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 4);
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "B"), "F");
    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "C"), 24);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(data_def_attribute_lookahead, incorrect_attribute_reference)
{
    std::string input(
        R"( 
&A SETA S'X
X DC C'A'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 0);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, lookup_of_two_refs_but_one_symbol_evaluation)
{
    std::string input(
        R"( 
&A SETA L'X+L'Y
X EQU 1,10
Y EQU 2,11
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 21);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, failed_lookup)
{
    std::string input(
        R"( 
&A SETA L'X+L'Y
X EQU 1,10
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 11);

    EXPECT_EQ(a.diags().size(), (size_t)1);
}

TEST(attribute_lookahead, nested_lookup)
{
    std::string input(
        R"( 
&V(1) SETC 'A','B','C'
&A SETA L'&V(L'X)
X EQU 1,2
B EQU 2,22
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 22);

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookahead_from_macro_bad_following_statement)
{
    std::string input(
        R"( 
 MACRO
 M
&A SETA L'A
 MEND

 M
 AGO .A
F 
A EQU 1,1,1
.A ANOP
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookahead_from_instruction_field)
{
    std::string input(
        R"( 
&A(1) SETC 'LR','SAM64','LR'
 &A(L'A) 
A EQU 1,2,1
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookahead_from_instruction_field_macro)
{
    std::string input(
        R"( 
 MACRO
 M
&A(1) SETC 'LR','SAM64','LR'
 &A(L'A) 
 MEND
 M
A EQU 1,2,1
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, lookahead_from_var_sym_label_index)
{
    std::string input(
        R"(&VAR(L'C) SETA 47

C DC C'STH'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
    auto var = a.context().hlasm_ctx->get_var_sym(id_index("VAR"));
    ASSERT_NE(var, nullptr);
    ASSERT_EQ(var->var_kind, variable_kind::SET_VAR_KIND);
    auto value = var->access_set_symbol_base()->access_set_symbol<int>()->get_value(3);

    EXPECT_EQ(value, 47);
}

TEST(EQU_attribute_lookahead, location_counter_use)
{
    std::string input(
        R"( 
  AIF (L'X EQ 2).NO
Y LR 1,1
X EQU 1,*-Y

  AIF (L'A EQ 4).NO
B LR 1,1
A DC AL(*-B+2)(*)
)");

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_properties(a.diags(),
        { std::pair<diagnostic_severity, size_t>(diagnostic_severity::warning, 5) },
        [](const auto& m) { return std::pair(m.severity, m.diag_range.start.line); }));
}

TEST(attribute_lookahead, ignore_invalid_code)
{
    std::string input = R"(
      AIF (L'C GT 0).SKIP
      'invalid
.SKIP ANOP
C     DC C'STH'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(a.diags().size(), (size_t)0);
}

TEST(attribute_lookahead, dc_in_copybook)
{
    std::string input =
        R"(
      GBLC &RESULT
&S    SETC 'STR'
      COPY LIB
*
&L    SETA L'LABEL
&T    SETC T'LABEL
&RESULT2 SETC '&L &T'
      END
)";

    std::string LIB =
        R"(
      MAC   LABEL
LABEL DC    C'&S'
)";

    std::string MAC = R"(   MACRO
      MAC &LBL
      GBLC &RESULT
&L    SETA L'&LBL
&T    SETC T'&LBL
&RESULT SETC '&L &T'
      MEND
)";

    mock_parse_lib_provider mock { { "LIB", LIB }, { "MAC", MAC } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W013" }));
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RESULT"), "1 U");
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RESULT2"), "3 C");
}

TEST(attribute_lookahead, mach_in_copybook)
{
    std::string input =
        R"(
      GBLC &RESULT
&S    SETC 'STR'
      COPY LIB
      END
)";

    std::string LIB =
        R"(
      MAC   LABEL
LABEL LLILF 0,C'&S'
)";

    std::string MAC = R"(   MACRO
      MAC &LBL
      GBLC &RESULT
&L    SETA L'&LBL
&T    SETC T'&LBL
&RESULT SETC '&L &T'
      MEND
)";

    mock_parse_lib_provider mock { { "LIB", LIB }, { "MAC", MAC } };
    analyzer a(input, analyzer_options { &mock });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RESULT"), "6 I");
}

TEST(attribute_lookahead, expression)
{
    std::string input = R"(
&X SETC 'A(3)'
&T SETC T'&X
A  DS   C
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "T"), "C");
}

TEST(lookahead, unused_duplicate_seq)
{
    std::string input = R"(
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(lookahead, hidden_duplicate_seq)
{
    std::string input = R"(
     AGO  .O
.A   ANOP
.X   ANOP
     AIF  (1).END
.O   ANOP
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
     AGO  .X
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(lookahead, used_duplicate_seq)
{
    std::string input = R"(
     AGO  .O
.A   ANOP
.X   ANOP
.X   ANOP
.X   ANOP
     AIF  (1).END
.O   ANOP
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
     AGO  .X
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E045", "E045" }));
}

TEST(lookahead, unconditional_redefinition)
{
    std::string input = R"(
     AGO  .O
.A   ANOP
.X   ANOP
     AIF  (1).END
.O   ANOP
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
.X   ANOP
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E045" }));
}

TEST(lookahead, unconditional_redefinition_2)
{
    std::string input = R"(
     AGO  .O
.A   ANOP
.X   ANOP
.X   ANOP
     AIF  (1).END
.O   ANOP
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
.X   ANOP
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E045" }));
}

TEST(lookahead, combined_redefinition)
{
    std::string input = R"(
     AGO  .O
.A   ANOP
.X   ANOP
.X   ANOP
.X   ANOP
     AIF  (1).END
.O   ANOP
     AIF  (1).B
.X   ANOP
.X   ANOP
.B   ANOP
.X   ANOP
     AGO  .X
.END END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E045", "E045", "E045" }));
}

TEST(lookahead, ord_with_aread)
{
    std::string input = R"(
      MACRO
      MAC
&X    AREAD
&X    AREAD
      MEND
*
      AIF   (L'X EQ 0).SKIP
      MAC
.TEST
.TEST
      MNOTE 'AAAA'
.SKIP ANOP
X     DS    C
      END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
}

TEST(lookahead, t_attr_special_case_skip_lookahead)
{
    std::string input = R"(
         MACRO
         MAC   &P
&X       SETB  (T'&P EQ 'O')
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(lookahead, t_attr_special_case_skip_lookahead_inverted)
{
    std::string input = R"(
         MACRO
         MAC   &P
&X       SETB  ('O' EQ T'&P)
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(lookahead, t_attr_special_case_skip_lookahead2)
{
    std::string input = R"(
         MACRO
         MAC   &P
         AIF   (T'&P EQ 'O').SKIP
.SKIP    ANOP
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(lookahead, t_attr_special_case_go_lookahead)
{
    std::string input = R"(
         MACRO
         MAC   &P
&X       SETB  (T'&P EQ 'C')
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W013" }));
}

TEST(lookahead, t_attr_special_case_go_lookahead2)
{
    std::string input = R"(
         MACRO
         MAC   &P
         AIF   (T'&P EQ 'C').SKIP
.SKIP    ANOP
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W013" }));
}

TEST(lookahead, t_attr_special_case_go_lookahead3)
{
    std::string input = R"(
         MACRO
         MAC   &P
         AIF   (T'&P EQ 'O'(L'TEST,*)).SKIP
.SKIP    ANOP
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W013", "W013" }));
}

TEST(lookahead, t_attr_special_case_go_lookahead4)
{
    std::string input = R"(
         MACRO
         MAC   &P
         AIF   (T'&P(L'TEST) EQ 'O').SKIP
.SKIP    ANOP
         MEND
.*
         MACRO
         MAC2
         AIF   (L'TEST EQ 5).SKIP
.SKIP    ANOP
         MEND
.*
         MAC   TEST
X        EQU   5
         MAC2
TEST     DS    CL(X)
         END
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "W013", "W013" }));
}

TEST(lookahead, seq_symbol_location)
{
    mock_parse_lib_provider libs {
        std::pair<std::string, std::string>("COPY1", R"(
         AGO   .INHERE1
.INHERE1 ANOP
)"),
        std::pair<std::string, std::string>("COPY2", R"(
         AIF   (L'X EQ 1).INHERE2

.INHERE2 ANOP
)"),
        std::pair<std::string, std::string>("MAC", R"( MACRO
         MAC
         AIF   (L'Y EQ 1).INMACRO

.INMACRO ANOP
         MEND
)"),
    };

    std::string input = R"(
         COPY  COPY1
         COPY  COPY2

         AIF   (1 EQ 0).OUTHERE1
.OUTHERE1 ANOP  ,
X        DS    C
         MAC
         AIF   (1 EQ 0).OUTHERE2
.OUTHERE2 ANOP  ,
Y        DS    C
)";

    const hlasm_plugin::utils::resource::resource_location copy1("COPY1"), copy2("COPY2"), mac("MAC"), opencode("OPEN");

    analyzer a(input, analyzer_options(opencode, &libs));
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    auto inhere1 = a.context().lsp_ctx->definition(copy1, { 1, 16 });
    auto inhere2 = a.context().lsp_ctx->definition(copy2, { 1, 30 });
    auto inmacro = a.context().lsp_ctx->definition(mac, { 2, 30 });
    auto outhere1 = a.context().lsp_ctx->definition(opencode, { 4, 30 });
    auto outhere2 = a.context().lsp_ctx->definition(opencode, { 8, 30 });

    EXPECT_EQ(inhere1, location(position(2, 0), copy1));
    EXPECT_EQ(inhere2, location(position(3, 0), copy2));
    EXPECT_EQ(inmacro, location(position(4, 0), mac));
    EXPECT_EQ(outhere1, location(position(5, 0), opencode));
    EXPECT_EQ(outhere2, location(position(9, 0), opencode));
}

TEST(lookahead, incomplete_data_definition)
{
    std::string input(
        R"(
&A SETA L'X
X  DS)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A010", "W013" }));
}

TEST(lookahead, invalid_equ_parameter_complex)
{
    std::string input(
        R"(
&A  SETA L'I
I   EQU  X(Y)
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A132" }));
}

TEST(lookahead, invalid_equ_parameter_string)
{
    std::string input(
        R"(
&A  SETA L'I
I   EQU  'ABC'
)");

    analyzer a(input);
    a.analyze();

    EXPECT_EQ(get_var_value<A_t>(a.hlasm_ctx(), "A"), 1);

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A132" }));
}

TEST(lookahead, p_attr_equ)
{
    std::string input = R"(
        AIF (0 EQ 0).SKIP
P       EQU 0,,,X'12345678'
.SKIP   ANOP
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_FALSE(get_symbol(a.hlasm_ctx(), "P"));
    const auto p = get_symbol_reference(a.hlasm_ctx(), "P");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->attributes().prog_type(), program_type(0x12345678));
}

TEST(lookahead, p_attr_dc)
{
    std::string input = R"(
        AIF (0 EQ 0).SKIP
P       DC  AP(X'12345678')(0)
.SKIP   ANOP
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_FALSE(get_symbol(a.hlasm_ctx(), "P"));
    const auto p = get_symbol_reference(a.hlasm_ctx(), "P");
    ASSERT_TRUE(p);

    EXPECT_EQ(p->attributes().prog_type(), program_type(0x12345678));
}

TEST(lookahead, a_attr)
{
    std::string input = R"(
        AIF (0 EQ 0).SKIP
AR0     EQU 0,,,,AR
.SKIP   ANOP
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());
    EXPECT_FALSE(get_symbol(a.hlasm_ctx(), "AR0"));
    const auto ar0 = get_symbol_reference(a.hlasm_ctx(), "AR0");
    ASSERT_TRUE(ar0);

    EXPECT_EQ(ar0->attributes().asm_type(), assembler_type::AR);
}

TEST(lookahead, sysattra_equ)
{
    std::string input = R"(
&NAME   SETC 'AR0'
&RES    SETC SYSATTRA('&NAME')
AR0     EQU 0,,,,AR
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "AR");
}

TEST(lookahead, sysattrp_equ)
{
    std::string input = R"(
&NAME   SETC 'P'
&RES    SETC SYSATTRP('&NAME')
P       EQU 0,,,C'ABCD'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "ABCD");
}

TEST(lookahead, sysattrp_dc)
{
    std::string input = R"(
&NAME   SETC 'P'
&RES    SETC SYSATTRP('&NAME')
P       DC   AP(C'ABCD')(0)
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    EXPECT_EQ(get_var_value<C_t>(a.hlasm_ctx(), "RES"), "ABCD");
}
