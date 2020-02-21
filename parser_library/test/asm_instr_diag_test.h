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

#pragma once
#include "common_testing.h"

TEST(diagnostics, org_incorrect_second_op)
{
	std::string input(
		R"( 
 ORG *,complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A115");
}

TEST(diagnostics, exitctl_op_incorrect_format)
{
	std::string input(
		R"( 
 EXITCTL SOURCE,complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A020");
}

TEST(diagnostics, exitctl_op_incorrect_value)
{
	std::string input(
		R"( 
 EXITCTL LISTING,not_number
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A131");
}

TEST(diagnostics, extrn_incorrect_part_operand)
{
	std::string input(
		R"( 
 EXTRN PART(,)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A129");
}

TEST(diagnostics, extrn_incorrect_complex_operand)
{
	std::string input(
		R"( 
 EXTRN complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A129");
}

TEST(diagnostics, ictl_empty_op)
{
	std::string input(
		R"( 
 ICTL , 
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A021");
}

TEST(diagnostics, ictl_undefined_op)
{
	std::string input(
		R"( 
 ICTL 1, 
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A242");
}

TEST(diagnostics, ictl_incorrect_begin_val)
{
	std::string input(
		R"( 
 ICTL 120
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A123");
}

TEST(diagnostics, ictl_incorrect_continuation_val)
{
	std::string input(
		R"( 
 ICTL 1,41,130
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A126");
}

TEST(diagnostics, ictl_incorrect_end_begin_diff)
{
	std::string input(
		R"( 
 ICTL 40,41
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A125");
}

TEST(diagnostics, ictl_incorrect_continuation_begin_diff)
{
	std::string input(
		R"( 
 ICTL 10,70,2
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A127");
}

TEST(diagnostics, end_incorrect_first_op_format)
{
	std::string input(
		R"( 
 END complex(operand) 
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A243");
}

TEST(diagnostics, end_incorrect_second_op_format)
{
	std::string input(
		R"( 
simple equ 2
 END ,simple
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A001");
}


TEST(diagnostics, end_incorrect_language_third_char)
{
	std::string input(
		R"( 
 END ,(one,four,three)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A140");
}

TEST(diagnostics, end_incorrect_language_second_char)
{
	std::string input(
		R"( 
 END ,(one,two,three)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A139");
}

TEST(diagnostics, end_incorrect_language_format)
{
	std::string input(
		R"( 
 END ,wrong(one,two,three)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A137");
}

TEST(diagnostics, drop_incorrect_op_format)
{
	std::string input(
		R"( 
 DROP complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A141");
}

TEST(diagnostics, cnop_incorrect_first_op_format)
{
	std::string input(
		R"( 
 CNOP complex(operand),3
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A143");
}

TEST(diagnostics, cnop_incorrect_second_op_format)
{
	std::string input(
		R"( 
 CNOP 10,complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A143");
}

TEST(diagnostics, cnop_incorrect_boundary)
{
	std::string input(
		R"( 
 CNOP 14,17
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A145");
}

TEST(diagnostics, ccw_unspecified_operand)
{
	std::string input(
		R"( 
  CCW ,,,
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A147");
}

TEST(diagnostics, ccw_incorrect_first_op)
{
	std::string input(
		R"( 
  CCW complex(operand),,,
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A143");
}

TEST(diagnostics, ccw_incorrect_second_op)
{
	std::string input(
		R"( 
  CCW 2,complex(operand),,
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A247");
}

TEST(diagnostics, space_incorrect_op_format)
{
	std::string input(
		R"( 
 SPACE complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A240");
}

TEST(diagnostics, space_incorrect_op_value)
{
	std::string input(
		R"( 
 SPACE -1
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A148");
}

TEST(diagnostics, cattr_incorrect_simple_format)
{
	std::string input(
		R"( 
 CATTR wrong
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A149");
}

TEST(diagnostics, cattr_incorrect_complex_format)
{
	std::string input(
		R"( 
 CATTR wrong(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A149");
}

TEST(diagnostics, cattr_incorrect_complex_params)
{
	std::string input(
		R"( 
 CATTR RMODE(one,two)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A016");
}

TEST(diagnostics, cattr_incorrect_rmode_param)
{
	std::string input(
		R"( 
 CATTR RMODE(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A204");
}

TEST(diagnostics, cattr_incorrect_align_param)
{
	std::string input(
		R"( 
 CATTR ALIGN(6)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A205");
}

TEST(diagnostics, cattr_incorrect_fill_param)
{
	std::string input(
		R"( 
 CATTR FILL(256)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A206");
}

TEST(diagnostics, cattr_incorrect_priority_param)
{
	std::string input(
		R"( 
 CATTR PRIORITY(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A208");
}

TEST(diagnostics, cattr_incorrect_part_param)
{
	std::string input(
		R"( 
 CATTR PART()
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A207");
}

TEST(diagnostics, cattr_empty_op)
{
	std::string input(
		R"( 
 CATTR ,NOLOAD
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A021");
}

TEST(diagnostics, ainsert_incorrect_string)
{
	std::string input(
		R"( 
 AINSERT one,two
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A301");
}

TEST(diagnostics, ainsert_incorrect_second_op)
{
	std::string input(
		R"( 
 AINSERT 'string',wrong
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A156");
}

TEST(diagnostics, adata_incorrect_op_format)
{
	std::string input(
		R"( 
 ADATA complex(operand),1,1,1,'string'
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A158");
}

TEST(diagnostics, adata_incorrect_last_op_format)
{
	std::string input(
		R"( 
 ADATA 1,2,3,4,complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A239");
}

TEST(diagnostics, adata_string_not_enclosed)
{
	std::string input(
		R"( 
 ADATA 1,2,3,4,string
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A300");
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
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A160");
}

TEST(diagnostics, acontrol_incorrect_simple_op_format)
{
	std::string input(
		R"( 
 ACONTROL wrong
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A161");
}

TEST(diagnostics, acontrol_incorrect_complex_op_format)
{
	std::string input(
		R"( 
 ACONTROL wrong(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A161");
}

TEST(diagnostics, acontrol_compat_format)
{
	std::string input(
		R"( 
 ACONTROL COMPAT(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A209");
}

TEST(diagnostics, acontrol_flag_format)
{
	std::string input(
		R"( 
 ACONTROL FLAG(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A211");
}

TEST(diagnostics, acontrol_optable_params_size)
{
	std::string input(
		R"( 
 ACONTROL OPTABLE(one,two,three)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A018");
}

TEST(diagnostics, acontrol_optable_first_params_format)
{
	std::string input(
		R"( 
 ACONTROL OPTABLE(one,two)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A212");
}

TEST(diagnostics, acontrol_optable_second_params_format)
{
	std::string input(
		R"( 
  ACONTROL OPTABLE(DOS,wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A213");
}

TEST(diagnostics, acontrol_typecheck_param)
{
	std::string input(
		R"( 
 ACONTROL TC(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A214");
}

TEST(diagnostics, acontrol_empty_op)
{
	std::string input(
		R"( 
 ACONTROL ,
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A021");
}


TEST(diagnostics, extrn_empty_op)
{
	std::string input(
		R"( 
 EXTRN ,
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A021");
}

TEST(diagnostics, xattr_scope_value)
{
	std::string input(
		R"( 
 XATTR SCOPE(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A200");
}

TEST(diagnostics, xattr_linkage_value)
{
	std::string input(
		R"( 
 XATTR LINKAGE(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A201");
}

TEST(diagnostics, xattr_reference_value)
{
	std::string input(
		R"( 
 XATTR REFERENCE(wrong)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A288");
}

TEST(diagnostics, xattr_reference_direct_indirect_options)
{
	std::string input(
		R"( 
 XATTR REFERENCE(DIRECT,INDIRECT)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A202");
}

TEST(diagnostics, xattr_reference_number_of_params)
{
	std::string input(
		R"( 
 XATTR REFERENCE(operand,operand,operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A018");
}

TEST(diagnostics, mnote_incorrect_message)
{
	std::string input(
		R"( 
 MNOTE complex(operand),'message'
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A119");
}

TEST(diagnostics, mnote_first_op_value)
{
	std::string input(
		R"( 
 MNOTE not_number,'message'
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A119");
}

TEST(diagnostics, mnote_first_op_format)
{
	std::string input(
		R"( 
 MNOTE complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A117");
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
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A117");
}

TEST(diagnostics, iseq_number_of_operands)
{
	std::string input(
		R"( 
 ISEQ 4
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A013");
}

TEST(diagnostics, iseq_incorrect_op_value)
{
	std::string input(
		R"( 
 ISEQ 1,200 
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A120");
}

TEST(diagnostics, push_print_specified)
{
	std::string input(
		R"( 
 PUSH PRINT,PRINT
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A112");
}

TEST(diagnostics, push_acontrol_specified)
{
	std::string input(
		R"( 
 PUSH ACONTROL,ACONTROL
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A112");
}

TEST(diagnostics, pop_noprint_first)
{
	std::string input(
		R"( 
 POP NOPRINT,PRINT
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A113");
}

TEST(diagnostics, pop_incorrect_last_operand)
{
	std::string input(
		R"( 
 POP wrong
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A110");
}

TEST(diagnostics, pop_only_noprint_specified)
{
	std::string input(
		R"( 
 POP NOPRINT
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A114");
}

TEST(diagnostics, push_incorrect_op_value)
{
	std::string input(
		R"( 
 PUSH wrong,ACONTROL
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A111");
}

TEST(diagnostics, org_incorrect_first_op)
{
	std::string input(
		R"( 
 ORG complex(operand)
)"
);
	analyzer a(input);
	a.analyze();
	dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->collect_diags();
	ASSERT_EQ(a.parser().getNumberOfSyntaxErrors(), (size_t)0);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().size(), (size_t)1);
	ASSERT_EQ(dynamic_cast<hlasm_plugin::parser_library::diagnosable*>(&a)->diags().at(0).code, "A245");
}
