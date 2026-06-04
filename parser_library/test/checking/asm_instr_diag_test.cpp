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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../output_handler_mock.h"

using namespace ::testing;

TEST(diagnostics, org_incorrect_second_op)
{
    std::string input(
        R"( 
 ORG *,complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A116", "S0002" }));
}

TEST(diagnostics, exitctl_op_incorrect_format)
{
    std::string input(
        R"( 
 EXITCTL SOURCE,complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A020" }));
}

TEST(diagnostics, exitctl_op_incorrect_value)
{
    std::string input(
        R"( 
 EXITCTL LISTING,not_number
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A131" }));
}

TEST(diagnostics, exitctl_overflow)
{
    std::string input(
        R"(
 EXITCTL LISTING,*+2147483648
 EXITCTL LISTING,2147483648
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A131", "A131" }));
}

TEST(diagnostics, extrn_incorrect_part_operand)
{
    std::string input(
        R"( 
 EXTRN PART(,)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A129" }));
}

TEST(diagnostics, extrn_incorrect_complex_operand)
{
    std::string input(
        R"( 
 EXTRN complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A129" }));
}

TEST(diagnostics, extrn_incorrect_part_type)
{
    std::string input(
        R"( 
 EXTRN PART(1)
)");
    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A129" }));
}

TEST(diagnostics, ictl_empty_op)
{
    std::string input(
        R"( 
 ICTL , 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A010" }));
}

TEST(diagnostics, ictl_undefined_op)
{
    std::string input(
        R"( 
 ICTL 1, 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A242" }));
}

TEST(diagnostics, ictl_incorrect_begin_val)
{
    std::string input(
        R"( 
 ICTL 120
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A123" }));
}

TEST(diagnostics, ictl_incorrect_continuation_val)
{
    std::string input(
        R"( 
 ICTL 1,41,130
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A126" }));
}

TEST(diagnostics, ictl_incorrect_end_begin_diff)
{
    std::string input(
        R"( 
 ICTL 40,41
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A125" }));
}

TEST(diagnostics, ictl_incorrect_continuation_begin_diff)
{
    std::string input(
        R"( 
 ICTL 10,70,2
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A127" }));
}

TEST(diagnostics, end_incorrect_first_op_format)
{
    std::string input(
        R"( 
 END complex(operand) 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, end_incorrect_second_op_format)
{
    std::string input(
        R"( 
simple equ 2
 END ,simple
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002" }));
}


TEST(diagnostics, end_incorrect_language_third_char)
{
    std::string input(
        R"( 
 END ,(one,four,toolong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A140" }));
}

TEST(diagnostics, end_incorrect_language_second_char)
{
    std::string input(
        R"( 
 END ,(one,two,three)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A139" }));
}

TEST(diagnostics, end_incorrect_language_format)
{
    std::string input(
        R"( 
 END ,wrong(one,two,three)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002" }));
}

TEST(diagnostics, drop_incorrect_op_format)
{
    std::string input(
        R"( 
 DROP complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "U003" }));
}

TEST(diagnostics, cnop_incorrect_first_op_format)
{
    std::string input(
        R"( 
 CNOP complex(operand),3
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, cnop_incorrect_second_op_format)
{
    std::string input(
        R"( 
 CNOP 10,complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, cnop_incorrect_boundary)
{
    std::string input(
        R"( 
 CNOP 14,17
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A145" }));
}

TEST(diagnostics, ccw_unspecified_operand)
{
    std::string input(
        R"( 
  CCW ,,,
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A147" }));
}

TEST(diagnostics, ccw_incorrect_first_op)
{
    std::string input(
        R"( 
  CCW complex(operand),,,
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, ccw_incorrect_second_op)
{
    std::string input(
        R"( 
  CCW 2,complex(operand),,
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, space_incorrect_op_format)
{
    std::string input(
        R"( 
 SPACE complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, space_incorrect_op_value)
{
    std::string input(
        R"( 
 SPACE -1
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A148" }));
}

TEST(diagnostics, cattr_incorrect_simple_format)
{
    std::string input(
        R"( 
  START
X CATTR wrong
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A149" }));
}

TEST(diagnostics, cattr_incorrect_complex_format)
{
    std::string input(
        R"( 
  START
X CATTR wrong(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A149" }));
}

TEST(diagnostics, cattr_incorrect_complex_params)
{
    std::string input(
        R"( 
  START
X CATTR RMODE(one,two)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A016" }));
}

TEST(diagnostics, cattr_incorrect_rmode_param)
{
    std::string input(
        R"( 
  START
X CATTR RMODE(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A204" }));
}

TEST(diagnostics, cattr_incorrect_align_param)
{
    std::string input(
        R"( 
  START
X CATTR ALIGN(6)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A205" }));
}

TEST(diagnostics, cattr_incorrect_fill_param)
{
    std::string input(
        R"( 
  START
X CATTR FILL(256)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A206" }));
}

TEST(diagnostics, cattr_incorrect_priority_param)
{
    std::string input(
        R"( 
  START
X CATTR PRIORITY(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A208" }));
}

TEST(diagnostics, cattr_incorrect_part_param)
{
    std::string input(
        R"( 
  START
X CATTR PART()
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A207" }));
}

TEST(diagnostics, cattr_incorrect_part_param2)
{
    std::string input(
        R"(
  START
X CATTR PART(1)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A207" }));
}

TEST(diagnostics, cattr_empty_op)
{
    std::string input(
        R"( 
  START
X CATTR ,NOLOAD
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A021" }));
}

TEST(diagnostics, cattr_missing_label_no_goff)
{
    std::string input = R"(
  START
  CATTR
)";
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A167" }));
}

TEST(diagnostics, cattr_missing_label_goff)
{
    std::string input = R"(
  START
  CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A167" }));
}

TEST(diagnostics, cattr_label_too_long)
{
    std::string input = R"(
  START
AAAAAAAAAAAAAAAAB CATTR
)";
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A167" }));
}

TEST(diagnostics, cattr_no_csect)
{
    std::string input = R"(
X CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A169" }));
}

TEST(diagnostics, cattr_redef)
{
    std::string input = R"(
    START
X   DS    0H
X   CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(diagnostics, cattr_csect_mismatch)
{
    std::string input = R"(
X   CSECT
X   CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A170" }));
}

TEST(diagnostics, cattr_symbol_redef)
{
    std::string input = R"(
    CSECT
X   DS  F
X   CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(diagnostics, cattr_csect_valid)
{
    std::string input = R"(
    START
X   CATTR
X   CSECT
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(diagnostics, cattr_ignored_params_attributes)
{
    std::string input = R"(
    START
X   CATTR
X   CATTR RMODE(31)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A171" }));
}

TEST(diagnostics, cattr_ignored_params_parts)
{
    std::string input = R"(
    START
X   CATTR
X   CATTR PART(Y)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A171" }));
}

TEST(diagnostics, cattr_missing_part)
{
    std::string input = R"(
    START
X   CATTR PART(Y)
X   CATTR
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A170" }));
}

TEST(diagnostics, cattr_part_class_redefine)
{
    std::string input = R"(
    START
X   CATTR PART(Y)
Z   CATTR PART(Y)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A170" }));
}

TEST(diagnostics, cattr_part_redefine)
{
    std::string input = R"(
    START
Y   DS  F
X   CATTR PART(Y)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E031" }));
}

TEST(diagnostics, ainsert_incorrect_string)
{
    std::string input(
        R"( 
 AINSERT one,back
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A301" }));
}

TEST(diagnostics, ainsert_incorrect_second_op)
{
    std::string input(
        R"( 
 AINSERT 'string',wrong
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A156" }));
}

TEST(diagnostics, adata_incorrect_op_format)
{
    std::string input(
        R"( 
 ADATA complex(operand),1,1,1,'string'
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

TEST(diagnostics, adata_incorrect_last_op_format)
{
    std::string input(
        R"( 
 ADATA 1,2,3,4,complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "A239", "E010" }));
}

TEST(diagnostics, adata_string_not_enclosed)
{
    std::string input(
        R"( 
 ADATA 1,2,3,4,string
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A239", "E010" }));
}

TEST(diagnostics, adata_string_too_long)
{
    std::string input(
        R"( 
 ADATA 1,2,3,4,'loremipsumdolorsitametloremipsumdolorsitametloremipsumsX
                loremipsumdolorsitametloremipsumdolorsitametloremipsumsX
                loremipsumdolorsitametloremipsumdolorsitametloremipsumsX
                loremipsumdolorsitametloremipsumdolorsitametloremipsumsX
                loremipsumdolorsitametloremipsumdolorsitametloremipsumsX
               '
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A160" }));
}

TEST(diagnostics, acontrol_incorrect_simple_op_format)
{
    std::string input(
        R"( 
 ACONTROL wrong
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A161" }));
}

TEST(diagnostics, acontrol_incorrect_complex_op_format)
{
    std::string input(
        R"( 
 ACONTROL wrong(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A161" }));
}

TEST(diagnostics, acontrol_compat_format)
{
    std::string input(
        R"( 
 ACONTROL COMPAT(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A209" }));
}

TEST(diagnostics, acontrol_flag_format)
{
    std::string input(
        R"( 
 ACONTROL FLAG(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A211" }));
}

TEST(diagnostics, acontrol_optable_params_size)
{
    std::string input(
        R"( 
 ACONTROL OPTABLE(one,two,three)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A018" }));
}

TEST(diagnostics, acontrol_optable_first_params_format)
{
    std::string input(
        R"( 
 ACONTROL OPTABLE(one,two)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A212" }));
}

TEST(diagnostics, acontrol_optable_second_params_format)
{
    std::string input(
        R"( 
  ACONTROL OPTABLE(DOS,wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A213" }));
}

TEST(diagnostics, acontrol_typecheck_param)
{
    std::string input(
        R"( 
 ACONTROL TC(wrong)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A214" }));
}

TEST(diagnostics, acontrol_empty_op)
{
    std::string input(
        R"( 
 ACONTROL ,
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A010" }));
}


TEST(diagnostics, extrn_empty_op)
{
    std::string input(
        R"( 
 EXTRN ,
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A010" }));
}

TEST(diagnostics, xattr_scope_value)
{
    std::string input(
        R"( 
X CSECT
X XATTR SCOPE(wrong)
)");
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A200" }));
}

TEST(diagnostics, xattr_linkage_value)
{
    std::string input(
        R"( 
X CSECT
X XATTR LINKAGE(wrong)
)");
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A201" }));
}

TEST(diagnostics, xattr_reference_value)
{
    std::string input(
        R"( 
X CSECT
X XATTR REFERENCE(wrong)
)");
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A288" }));
}

TEST(diagnostics, xattr_reference_direct_indirect_options)
{
    std::string input(
        R"( 
X CSECT
X XATTR REFERENCE(DIRECT,INDIRECT)
)");
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A202" }));
}

TEST(diagnostics, xattr_reference_number_of_params)
{
    std::string input(
        R"( 
X CSECT
X XATTR REFERENCE(operand,operand,operand)
)");
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A018" }));
}

TEST(diagnostics, xattr_requires_goff)
{
    std::string input = R"(
  EXTRN X
X XATTR LINKAGE(OS)
)";
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A166" }));
}

TEST(diagnostics, xattr_requires_label)
{
    std::string input = R"(
  EXTRN X
  XATTR LINKAGE(OS)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A168" }));
}

TEST(diagnostics, xattr_psect_repeated)
{
    std::string input = R"(
C CSECT
P CSECT
C XATTR PSECT(P)
C XATTR PSECT(P)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A172" }));
}

TEST(diagnostics, xattr_psect_missing)
{
    std::string input = R"(
C   CSECT
C   XATTR PSECT(P)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "E032" }));
}

TEST(diagnostics, xattr_psect_valid_ref)
{
    std::string input = R"(
C   CSECT
C   XATTR PSECT(Q)
P   CATTR
Q   DS  H
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(diagnostics, xattr_psect_invalid_ref)
{
    std::string input = R"(
C   CSECT
C   XATTR PSECT(R)
P   CATTR
Q   DS  H
R   EQU Q
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A173" }));
}

TEST(diagnostics, xattr_psect_invalid_ref2)
{
    std::string input = R"(
C   CSECT
C   XATTR PSECT(1)
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A248" }));
}

TEST(diagnostics, xattr_psect_incompatible)
{
    std::string input = R"(
C   CSECT
C   XATTR PSECT(P)
P   COM
)";
    analyzer a(input, analyzer_options(asm_option { .sysopt_xobject = true }));
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A173" }));
}

TEST(diagnostics, mnote_incorrect_message)
{
    std::string input(
        R"( 
 MNOTE complex(operand),'message'
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A119" }));
}

TEST(diagnostics, mnote_first_op_value)
{
    std::string input(
        R"( 
 MNOTE not_number,'message'
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A119" }));
}

TEST(diagnostics, mnote_first_op_format)
{
    std::string input(
        R"( 
 MNOTE complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A300" }));
}

TEST(diagnostics, mnote_long_message)
{
    std::string input(
        R"( 
 MNOTE 'extremely_long_character_sequence_that_is_over_the_allowed_charX
               limit_loremipsumdolorsitamet_loremipsumdolorsitametloremX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolorX
               ipsumdolorsitamet_loremipsumdolorsitamet_loremipsumdolo'
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A117", "MNOTE" }));
}

TEST(diagnostics, iseq_number_of_operands)
{
    std::string input(
        R"( 
 ISEQ 4
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A013" }));
}

TEST(diagnostics, iseq_incorrect_op_value)
{
    std::string input(
        R"( 
 ISEQ 1,200 
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A120" }));
}

TEST(diagnostics, push_print_specified)
{
    std::string input(
        R"( 
 PUSH PRINT,PRINT
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A112" }));
}

TEST(diagnostics, push_acontrol_specified)
{
    std::string input(
        R"( 
 PUSH ACONTROL,ACONTROL
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A112" }));
}

TEST(diagnostics, pop_noprint_first)
{
    std::string input(
        R"( 
 POP NOPRINT,PRINT
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A113" }));
}

TEST(diagnostics, pop_incorrect_last_operand)
{
    std::string input(
        R"( 
 POP wrong
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A110" }));
}

TEST(diagnostics, pop_only_noprint_specified)
{
    std::string input(
        R"( 
 POP NOPRINT
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A114" }));
}

TEST(diagnostics, push_incorrect_op_value)
{
    std::string input(
        R"( 
 PUSH wrong,ACONTROL
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "A111" }));
}

TEST(diagnostics, org_incorrect_first_op)
{
    std::string input(
        R"( 
 ORG complex(operand)
)");
    analyzer a(input);
    a.analyze();
    EXPECT_TRUE(matches_message_codes(a.diags(), { "S0002", "E010" }));
}

struct mnote_test
{
    int code;
    std::string text;
    diagnostic_severity expected;
};

class mnote_fixture : public ::testing::TestWithParam<mnote_test>
{};

INSTANTIATE_TEST_SUITE_P(mnote,
    mnote_fixture,
    ::testing::Values(mnote_test { -2, "test", diagnostic_severity::hint },
        mnote_test { -1, "test", diagnostic_severity::hint },
        mnote_test { 0, "test", diagnostic_severity::hint },
        mnote_test { 1, "test", diagnostic_severity::hint },
        mnote_test { 2, "test", diagnostic_severity::info },
        mnote_test { 3, "test", diagnostic_severity::info },
        mnote_test { 4, "test", diagnostic_severity::warning },
        mnote_test { 5, "test", diagnostic_severity::warning },
        mnote_test { 6, "test", diagnostic_severity::warning },
        mnote_test { 7, "test", diagnostic_severity::warning },
        mnote_test { 8, "test", diagnostic_severity::error },
        mnote_test { 20, "test", diagnostic_severity::error },
        mnote_test { 150, "test", diagnostic_severity::error },
        mnote_test { 255, "test", diagnostic_severity::error }));

constexpr auto proj_cms = [](const auto& m) { return std::make_tuple(m.code, m.message, m.severity); };

TEST_P(mnote_fixture, diagnostic_severity)
{
    const auto& [code, text, expected] = GetParam();
    std::string input = " MNOTE "
        + (code == -2        ? ""
                : code == -1 ? "*,"
                             : std::to_string(code) + ",")
        + "'" + text + "'";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_properties(a.diags(), { std::make_tuple("MNOTE", text, expected) }, proj_cms));
}

TEST(mnote, substitution_first)
{
    std::string input = R"(
&L  SETA  4
    MNOTE &L,'test message'
)";

    analyzer a(input);
    a.analyze();

    static constexpr auto expected = std::make_tuple("MNOTE", "test message", diagnostic_severity::warning);
    EXPECT_TRUE(matches_message_properties(a.diags(), { expected }, proj_cms));
}

TEST(mnote, substitution_both)
{
    std::string input = R"(
&L  SETA  8
&M  SETC  'test message'
    MNOTE &L,'&M'
)";

    analyzer a(input);
    a.analyze();

    static constexpr auto expected = std::make_tuple("MNOTE", "test message", diagnostic_severity::error);
    EXPECT_TRUE(matches_message_properties(a.diags(), { expected }, proj_cms));
}

TEST(mnote, empty_first_arg)
{
    std::string input = R"(
    MNOTE ,'test message'
)";

    analyzer a(input);
    a.analyze();

    static constexpr auto expected = std::make_tuple("MNOTE", "test message", diagnostic_severity::hint);
    EXPECT_TRUE(matches_message_properties(a.diags(), { expected }, proj_cms));
}

TEST(mnote, three_args)
{
    std::string input = R"(
    MNOTE ,'test message',
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A012" }));
}

TEST(mnote, emtpy_second_arg)
{
    std::string input = R"(
    MNOTE 0,
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE", "A300" }));
}

TEST(mnote, missing_quotes)
{
    std::string input = R"(
    MNOTE 0,test
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A300" }));
}

TEST(mnote, nonprintable_characters)
{
    std::string input = R"(
&C  SETC X2C('0101')
    MNOTE 0,'&C'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_message_text(a.diags(), { "<01><01>" }));
}

TEST(mnote, empty)
{
    std::string input = R"(
&C  SETC ''
    MNOTE 0,'&C'
)";

    analyzer a(input);
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
    EXPECT_TRUE(matches_message_properties(a.diags(), { range({ 2, 12 }, { 2, 16 }) }, &diagnostic::diag_range));
}

TEST(mnote, output)
{
    std::string input = " MNOTE 1,'test string'";

    NiceMock<output_hanler_mock> output;

    analyzer a(input, analyzer_options(&output));

    EXPECT_CALL(output, mnote(1, StrEq("test string")));

    a.analyze();
}

TEST(print, simple_op)
{
    std::string input = " PRINT GEN";

    analyzer a(input);

    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(print, wrong_simple_op)
{
    std::string input = " PRINT WRONG";

    analyzer a(input);

    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A109" }));
}

TEST(print, complex_op)
{
    std::string input = " PRINT COMPLEX(OP)";

    analyzer a(input);

    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A109" }));
}

TEST(print, error_on_empty)
{
    std::string input = " PRINT ,";

    analyzer a(input);

    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "A010" }));
}

TEST(print, tolerate_null)
{
    std::string input = " PRINT ,,,,";

    analyzer a(input);

    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
