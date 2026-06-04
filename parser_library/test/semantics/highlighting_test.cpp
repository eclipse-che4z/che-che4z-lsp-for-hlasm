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

#include <algorithm>
#include <ostream>
#include <string>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../gtest_stringers.h"
#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "preprocessor_options.h"
#include "protocol.h"
#include "semantics/highlighting_info.h"
#include "utils/resource_location.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;
using namespace hlasm_plugin::utils::resource;

namespace std {

inline void PrintTo(const lines_info& tokens, std::ostream* os)
{
    for (const token_info& t : tokens)
    {
        *os << "{ " << t.token_range << ", " << (std::underlying_type_t<hl_scopes>)t.scope << " },\n";
    }
}
} // namespace std

namespace {
const auto source_file_loc = resource_location("file_name");
}

TEST(highlighting, simple)
{
    const std::string contents = "A EQU 1";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 0 }, { 0, 1 } }, hl_scopes::label),
        token_info({ { 0, 2 }, { 0, 5 } }, hl_scopes::instruction),
        token_info({ { 0, 6 }, { 0, 7 } }, hl_scopes::number) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_noop)
{
    const std::string contents = " SAM31 remark";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 0, 1 }, { 0, 6 } }, hl_scopes::instruction),
        token_info({ { 0, 7 }, { 0, 13 } }, hl_scopes::remark),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_noop_multiline)
{
    const std::string contents = R"(
    SAM31 remark                                                       X
               remark2
)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 1, 4 }, { 1, 9 } }, hl_scopes::instruction),
        token_info({ { 1, 10 }, { 1, 71 } }, hl_scopes::remark),
        token_info({ { 1, 71 }, { 1, 72 } }, hl_scopes::continuation),
        token_info({ { 2, 0 }, { 2, 15 } }, hl_scopes::ignored),
        token_info({ { 2, 15 }, { 2, 22 } }, hl_scopes::remark),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_expr)
{
    const std::string contents = " LR 1*1+X,L'X";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 3 } }, hl_scopes::instruction),
        token_info({ { 0, 4 }, { 0, 5 } }, hl_scopes::number),
        token_info({ { 0, 5 }, { 0, 6 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 6 }, { 0, 7 } }, hl_scopes::number),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 8 }, { 0, 9 } }, hl_scopes::ordinary_symbol),
        token_info({ { 0, 9 }, { 0, 10 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 10 }, { 0, 11 } }, hl_scopes::data_attr_type),
        token_info({ { 0, 11 }, { 0, 12 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 12 }, { 0, 13 } }, hl_scopes::ordinary_symbol) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_expr_2)
{
    const std::string contents = " L X'F',*";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 2 } }, hl_scopes::instruction),
        token_info({ { 0, 3 }, { 0, 4 } }, hl_scopes::self_def_type),
        token_info({ { 0, 4 }, { 0, 7 } }, hl_scopes::string),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 8 }, { 0, 9 } }, hl_scopes::operand) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_expr_3)
{
    const std::string contents = " L 1,=C'1'";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 2 } }, hl_scopes::instruction),
        token_info({ { 0, 3 }, { 0, 4 } }, hl_scopes::number),
        token_info({ { 0, 4 }, { 0, 5 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 5 }, { 0, 6 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 6 }, { 0, 7 } }, hl_scopes::data_def_type),
        token_info({ { 0, 7 }, { 0, 10 } }, hl_scopes::string) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, data_def)
{
    const std::string contents = " DC 4CAP8L4''";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 3 } }, hl_scopes::instruction),
        token_info({ { 0, 4 }, { 0, 5 } }, hl_scopes::number),
        token_info({ { 0, 5 }, { 0, 7 } }, hl_scopes::data_def_type),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::data_def_modifier),
        token_info({ { 0, 8 }, { 0, 9 } }, hl_scopes::number),
        token_info({ { 0, 9 }, { 0, 10 } }, hl_scopes::data_def_modifier),
        token_info({ { 0, 10 }, { 0, 11 } }, hl_scopes::number),
        token_info({ { 0, 11 }, { 0, 13 } }, hl_scopes::string) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, asm_simple_operand)
{
    const std::string contents = " AMODE ANY64";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 0, 1 }, { 0, 6 } }, hl_scopes::instruction),
        token_info({ { 0, 7 }, { 0, 12 } }, hl_scopes::operand),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, asm_list)
{
    const std::string contents = " USING (op2,op3)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 6 } }, hl_scopes::instruction),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 8 }, { 0, 11 } }, hl_scopes::ordinary_symbol),
        token_info({ { 0, 11 }, { 0, 12 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 12 }, { 0, 15 } }, hl_scopes::ordinary_symbol),
        token_info({ { 0, 15 }, { 0, 16 } }, hl_scopes::operator_symbol) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, asm_list_2)
{
    const std::string contents = " AMODE op1(op2,op3)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 6 } }, hl_scopes::instruction),
        token_info({ { 0, 7 }, { 0, 10 } }, hl_scopes::operand),
        token_info({ { 0, 10 }, { 0, 11 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 11 }, { 0, 14 } }, hl_scopes::operand),
        token_info({ { 0, 14 }, { 0, 15 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 15 }, { 0, 18 } }, hl_scopes::operand),
        token_info({ { 0, 18 }, { 0, 19 } }, hl_scopes::operator_symbol) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, continuation)
{
    const std::string contents =
        R"(D EQU                                                                 1Xignored
IgnoredIgnoredI1 remark)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 0 }, { 0, 1 } }, hl_scopes::label),
        token_info({ { 0, 2 }, { 0, 5 } }, hl_scopes::instruction),
        token_info({ { 0, 70 }, { 0, 71 } }, hl_scopes::number),
        token_info({ { 0, 71 }, { 0, 72 } }, hl_scopes::continuation),
        token_info({ { 0, 72 }, { 0, 79 } }, hl_scopes::ignored),
        token_info({ { 1, 0 }, { 1, 15 } }, hl_scopes::ignored),
        token_info({ { 1, 15 }, { 1, 16 } }, hl_scopes::number),
        token_info({ { 1, 17 }, { 1, 23 } }, hl_scopes::remark) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, macro_alternative_continuation)
{
    const std::string contents = R"(
 MACRO
 MAC
 MEND
 MAC OP1, remark                                                       X
               OP2 remark2)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 1, 1 }, { 1, 6 } }, hl_scopes::instruction),
        token_info({ { 2, 1 }, { 2, 4 } }, hl_scopes::instruction),
        token_info({ { 3, 1 }, { 3, 5 } }, hl_scopes::instruction),
        token_info({ { 4, 1 }, { 4, 4 } }, hl_scopes::instruction),
        token_info({ { 4, 5 }, { 4, 8 } }, hl_scopes::operand),
        token_info({ { 4, 8 }, { 4, 9 } }, hl_scopes::operator_symbol),
        token_info({ { 4, 10 }, { 4, 71 } }, hl_scopes::remark),
        token_info({ { 4, 71 }, { 4, 72 } }, hl_scopes::continuation),
        token_info({ { 5, 0 }, { 5, 15 } }, hl_scopes::ignored),
        token_info({ { 5, 15 }, { 5, 18 } }, hl_scopes::operand),
        token_info({ { 5, 19 }, { 5, 26 } }, hl_scopes::remark) };

    EXPECT_EQ(tokens, expected);
}


TEST(highlighting, var_sym_array_subscript)
{
    const std::string contents = "&VARP(31+L'C) SETA 45\n\nC EQU 1";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 0 }, { 0, 5 } }, hl_scopes::var_symbol),
        token_info({ { 0, 5 }, { 0, 6 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 6 }, { 0, 8 } }, hl_scopes::number),
        token_info({ { 0, 8 }, { 0, 9 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 9 }, { 0, 10 } }, hl_scopes::data_attr_type),
        token_info({ { 0, 10 }, { 0, 11 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 11 }, { 0, 12 } }, hl_scopes::ordinary_symbol),
        token_info({ { 0, 12 }, { 0, 13 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 14 }, { 0, 18 } }, hl_scopes::instruction),
        token_info({ { 0, 19 }, { 0, 21 } }, hl_scopes::number),
        token_info({ { 2, 0 }, { 2, 1 } }, hl_scopes::label),
        token_info({ { 2, 2 }, { 2, 5 } }, hl_scopes::instruction),
        token_info({ { 2, 6 }, { 2, 7 } }, hl_scopes::number) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, ca_expr)
{
    const std::string contents = " AIF (T'&SYSDATC EQ 'C').LOOP";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 4 } }, hl_scopes::instruction),
        token_info({ { 0, 5 }, { 0, 6 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 6 }, { 0, 7 } }, hl_scopes::data_attr_type),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 8 }, { 0, 16 } }, hl_scopes::var_symbol),
        token_info({ { 0, 17 }, { 0, 19 } }, hl_scopes::operand),
        token_info({ { 0, 20 }, { 0, 23 } }, hl_scopes::string),
        token_info({ { 0, 23 }, { 0, 24 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 24 }, { 0, 29 } }, hl_scopes::seq_symbol) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, aread)
{
    const std::string contents = R"(
 MACRO
 MAC
&C AREAD
 MEND
 MAC
aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbb
)";
    analyzer a(contents, analyzer_options { collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 1, 1 }, { 1, 6 } }, hl_scopes::instruction),
        token_info({ { 2, 1 }, { 2, 4 } }, hl_scopes::instruction),
        token_info({ { 3, 0 }, { 3, 2 } }, hl_scopes::var_symbol),
        token_info({ { 3, 3 }, { 3, 8 } }, hl_scopes::instruction),
        token_info({ { 4, 1 }, { 4, 5 } }, hl_scopes::instruction),
        token_info({ { 5, 1 }, { 5, 4 } }, hl_scopes::instruction),
        token_info({ { 6, 0 }, { 6, 80 } }, hl_scopes::string),
        token_info({ { 6, 80 }, { 6, 90 } }, hl_scopes::ignored),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, single_chars)
{
    const std::string contents = R"(
&C SETC 'A'
&C SETC 'B'
&C SETC 'C'
&C SETC 'D'
&C SETC 'E'
&C SETC 'F'
&C SETC 'G'
&C SETC 'H'
&C SETC 'I'
&C SETC 'J'
&C SETC 'K'
&C SETC 'L'
&C SETC 'M'
&C SETC 'N'
&C SETC 'O'
&C SETC 'P'
&C SETC 'Q'
&C SETC 'R'
&C SETC 'S'
&C SETC 'T'
&C SETC 'U'
&C SETC 'V'
&C SETC 'W'
&C SETC 'X'
&C SETC 'Y'
&C SETC 'Z'
)";
    analyzer a(contents, analyzer_options { source_file_loc, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected;

    for (size_t i = 1; i <= 'Z' - 'A' + 1; ++i)
    {
        expected.emplace_back(token_info({ { i, 0 }, { i, 2 } }, hl_scopes::var_symbol));
        expected.emplace_back(token_info({ { i, 3 }, { i, 7 } }, hl_scopes::instruction));
        expected.emplace_back(token_info({ { i, 8 }, { i, 11 } }, hl_scopes::string));
    }

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, multiline_macro_param)
{
    const std::string contents = R"(
        MACRO
        MAC
        MEND

        MAC   (L1,                     comment                         X
               L2,                     comment                         X
               L3,                     comment                         X
               L4)                     comment                         
)";
    analyzer a(contents, analyzer_options { collect_highlighting_info::yes });
    a.analyze();

    EXPECT_TRUE(a.diags().empty());

    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 1, 8 }, { 1, 13 } }, hl_scopes::instruction),
        token_info({ { 2, 8 }, { 2, 11 } }, hl_scopes::instruction),
        token_info({ { 3, 8 }, { 3, 12 } }, hl_scopes::instruction),

        token_info({ { 5, 8 }, { 5, 11 } }, hl_scopes::instruction),

        token_info({ { 5, 14 }, { 5, 15 } }, hl_scopes::operator_symbol),

        token_info({ { 5, 15 }, { 5, 17 } }, hl_scopes::operand),
        token_info({ { 5, 17 }, { 5, 18 } }, hl_scopes::operator_symbol),
        token_info({ { 5, 39 }, { 5, 71 } }, hl_scopes::remark),

        token_info({ { 5, 71 }, { 5, 72 } }, hl_scopes::continuation),
        token_info({ { 6, 0 }, { 6, 15 } }, hl_scopes::ignored),

        token_info({ { 6, 15 }, { 6, 17 } }, hl_scopes::operand),
        token_info({ { 6, 17 }, { 6, 18 } }, hl_scopes::operator_symbol),
        token_info({ { 6, 39 }, { 6, 71 } }, hl_scopes::remark),

        token_info({ { 6, 71 }, { 6, 72 } }, hl_scopes::continuation),
        token_info({ { 7, 0 }, { 7, 15 } }, hl_scopes::ignored),

        token_info({ { 7, 15 }, { 7, 17 } }, hl_scopes::operand),
        token_info({ { 7, 17 }, { 7, 18 } }, hl_scopes::operator_symbol),
        token_info({ { 7, 39 }, { 7, 71 } }, hl_scopes::remark),

        token_info({ { 7, 71 }, { 7, 72 } }, hl_scopes::continuation),
        token_info({ { 8, 0 }, { 8, 15 } }, hl_scopes::ignored),

        token_info({ { 8, 15 }, { 8, 17 } }, hl_scopes::operand),
        token_info({ { 8, 17 }, { 8, 18 } }, hl_scopes::operator_symbol),
        token_info({ { 8, 39 }, { 8, 71 } }, hl_scopes::remark),
    };

    EXPECT_EQ(tokens, expected);
}

namespace {
struct test_params
{
    std::string text_to_test;
    semantics::lines_info expected;
};

class highlighting_fixture : public ::testing::TestWithParam<test_params>
{};

INSTANTIATE_TEST_SUITE_P(highlighting,
    highlighting_fixture,
    ::testing::Values(
        test_params {
            R"(
        MAC   (L1,                     comment                         X
               L2,                     comment                         X
               L3,                     comment                         X
               L4)                     comment
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 15 } }, hl_scopes::operator_symbol),
                token_info({ { 1, 15 }, { 1, 17 } }, hl_scopes::operand),
                token_info({ { 1, 17 }, { 1, 18 } }, hl_scopes::operator_symbol),
                token_info({ { 1, 39 }, { 1, 71 } }, hl_scopes::remark),
                token_info({ { 1, 71 }, { 1, 72 } }, hl_scopes::continuation),

                token_info({ { 2, 0 }, { 2, 15 } }, hl_scopes::ignored),
                token_info({ { 2, 15 }, { 2, 17 } }, hl_scopes::operand),
                token_info({ { 2, 17 }, { 2, 18 } }, hl_scopes::operator_symbol),
                token_info({ { 2, 39 }, { 2, 71 } }, hl_scopes::remark),
                token_info({ { 2, 71 }, { 2, 72 } }, hl_scopes::continuation),

                token_info({ { 3, 0 }, { 3, 15 } }, hl_scopes::ignored),
                token_info({ { 3, 15 }, { 3, 17 } }, hl_scopes::operand),
                token_info({ { 3, 17 }, { 3, 18 } }, hl_scopes::operator_symbol),
                token_info({ { 3, 39 }, { 3, 71 } }, hl_scopes::remark),
                token_info({ { 3, 71 }, { 3, 72 } }, hl_scopes::continuation),

                token_info({ { 4, 0 }, { 4, 15 } }, hl_scopes::ignored),
                token_info({ { 4, 15 }, { 4, 17 } }, hl_scopes::operand),
                token_info({ { 4, 17 }, { 4, 18 } }, hl_scopes::operator_symbol),
                token_info({ { 4, 39 }, { 4, 46 } }, hl_scopes::remark),
            },
        },
        test_params {
            R"(
        MAC   (A                                                       X
               ,X)
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 15 } }, hl_scopes::operator_symbol),
                token_info({ { 1, 15 }, { 1, 16 } }, hl_scopes::operand),
                token_info({ { 1, 71 }, { 1, 72 } }, hl_scopes::continuation),

                token_info({ { 2, 0 }, { 2, 15 } }, hl_scopes::ignored),
                token_info({ { 2, 15 }, { 2, 18 } }, hl_scopes::remark),
            },
        },
        test_params {
            R"(
        MAC   L1
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 16 } }, hl_scopes::operand),
            },
        },
        test_params {
            R"(
        MAC   ( L1
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 15 } }, hl_scopes::operator_symbol),
                token_info({ { 1, 16 }, { 1, 18 } }, hl_scopes::remark),
            },
        },
        test_params {
            R"(
        MAC   L1)
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 16 } }, hl_scopes::operand),
            },
        },
        test_params {
            R"(
        MAC   'L1
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 17 } }, hl_scopes::operand),
            },
        },
        test_params {
            R"(
        MAC   L'
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 16 } }, hl_scopes::operand),
            },
        },
        test_params {
            R"(
        MAC   &
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),
            },
        },
        test_params {
            R"(
        MAC   &VAR
)",
            semantics::lines_info {
                token_info({ { 1, 8 }, { 1, 11 } }, hl_scopes::instruction),

                token_info({ { 1, 14 }, { 1, 18 } }, hl_scopes::var_symbol),
            },
        }));

} // namespace

TEST_P(highlighting_fixture, macro_params_no_definition)
{
    analyzer a(GetParam().text_to_test, analyzer_options { collect_highlighting_info::yes });
    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "E049" }));

    EXPECT_EQ(a.take_semantic_tokens(), GetParam().expected);
}

TEST(highlighting, endevor_preprocessor_statement)
{
    const std::string contents = R"(
-INC  MEMBER bla bla)";

    analyzer a(
        contents, analyzer_options { source_file_loc, endevor_preprocessor_options(), collect_highlighting_info::yes });
    a.analyze();

    const auto& tokens = a.take_semantic_tokens();
    semantics::lines_info expected = {
        token_info({ { 1, 0 }, { 1, 4 } }, hl_scopes::instruction),
        token_info({ { 1, 6 }, { 1, 12 } }, hl_scopes::operand),
        token_info({ { 1, 13 }, { 1, 20 } }, hl_scopes::remark),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, cics_preprocessor_statement)
{
    const std::string contents = R"(
A   EXEC CICS ABEND ABCODE('1234')                                      00000000
    EXEC CICS ABEND                                                    X
 ABCODE('1234')
    EXEC CICS ABEND ABCODE('12                                         X00000001
 34') NODUMP

B   L 0,DFHRESP ( NORMAL ) bla                                         X00000002
               bla                                                     XYZ
               bla
                                                                       X00000004
               L 1,DFHRESP(NORMAL))";

    analyzer a(
        contents, analyzer_options { source_file_loc, cics_preprocessor_options(), collect_highlighting_info::yes });
    a.analyze();

    const auto& tokens = a.take_semantic_tokens();
    const semantics::lines_info expected = {
        token_info({ { 1, 0 }, { 1, 1 } }, hl_scopes::label),
        token_info({ { 1, 4 }, { 1, 19 } }, hl_scopes::instruction),
        token_info({ { 1, 20 }, { 1, 34 } }, hl_scopes::operand),
        token_info({ { 1, 72 }, { 1, 80 } }, hl_scopes::ignored),
        token_info({ { 2, 4 }, { 2, 19 } }, hl_scopes::instruction),
        token_info({ { 2, 71 }, { 2, 72 } }, hl_scopes::continuation),
        token_info({ { 3, 1 }, { 3, 15 } }, hl_scopes::operand),
        token_info({ { 4, 4 }, { 4, 19 } }, hl_scopes::instruction),
        token_info({ { 4, 20 }, { 4, 71 } }, hl_scopes::operand),
        token_info({ { 4, 71 }, { 4, 72 } }, hl_scopes::continuation),
        token_info({ { 4, 72 }, { 4, 80 } }, hl_scopes::ignored),
        token_info({ { 5, 1 }, { 5, 5 } }, hl_scopes::operand),
        token_info({ { 5, 6 }, { 5, 12 } }, hl_scopes::operand),

        token_info({ { 7, 0 }, { 7, 1 } }, hl_scopes::label),
        token_info({ { 7, 4 }, { 7, 5 } }, hl_scopes::instruction),
        token_info({ { 7, 6 }, { 7, 7 } }, hl_scopes::operand),
        token_info({ { 7, 8 }, { 7, 26 } }, hl_scopes::operand),
        token_info({ { 7, 26 }, { 7, 71 } }, hl_scopes::remark),
        token_info({ { 7, 71 }, { 7, 72 } }, hl_scopes::continuation),
        token_info({ { 7, 72 }, { 7, 80 } }, hl_scopes::ignored),
        token_info({ { 8, 15 }, { 8, 71 } }, hl_scopes::remark),
        token_info({ { 8, 71 }, { 8, 72 } }, hl_scopes::continuation),
        token_info({ { 8, 72 }, { 8, 74 } }, hl_scopes::ignored),
        token_info({ { 9, 15 }, { 9, 18 } }, hl_scopes::remark),
        token_info({ { 10, 71 }, { 10, 72 } }, hl_scopes::continuation),
        token_info({ { 10, 72 }, { 10, 80 } }, hl_scopes::ignored),
        token_info({ { 11, 15 }, { 11, 16 } }, hl_scopes::instruction),
        token_info({ { 11, 17 }, { 11, 18 } }, hl_scopes::operand),
        token_info({ { 11, 19 }, { 11, 34 } }, hl_scopes::operand),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, db2_preprocessor_statement_include)
{
    const std::string contents = R"(
AAA EXEC  SQL   INCLUDE  SQLCA -- REMARK                                00000001
                                                         EXEC SQL --REMX00000020
               INCLUDE SQLCA         -- rem  rem2                       00000300
                  EXEC      SQL                                        X00004000
               SELECT                                                  X
               1       --rem                                           X00050000
                   INTO :B                                             X
               FROM                                                    X
               SYSIBM.SYSDUMMY1
B SQL  TYPE   IS RESULT_SET_LOCATOR VARYING   comment comment2          006)";

    analyzer a(
        contents, analyzer_options { source_file_loc, db2_preprocessor_options(), collect_highlighting_info::yes });
    a.analyze();

    const auto& tokens = a.take_semantic_tokens();
    const semantics::lines_info expected = {
        token_info({ { 1, 0 }, { 1, 3 } }, hl_scopes::label),
        token_info({ { 1, 4 }, { 1, 13 } }, hl_scopes::instruction),
        token_info({ { 1, 13 }, { 1, 31 } }, hl_scopes::operand),
        token_info({ { 1, 31 }, { 1, 71 } }, hl_scopes::remark),
        token_info({ { 1, 72 }, { 1, 80 } }, hl_scopes::ignored),
        token_info({ { 2, 57 }, { 2, 65 } }, hl_scopes::instruction),
        token_info({ { 2, 65 }, { 2, 66 } }, hl_scopes::operand),
        token_info({ { 2, 66 }, { 2, 71 } }, hl_scopes::remark),
        token_info({ { 2, 71 }, { 2, 72 } }, hl_scopes::continuation),
        token_info({ { 2, 72 }, { 2, 80 } }, hl_scopes::ignored),
        token_info({ { 3, 15 }, { 3, 37 } }, hl_scopes::operand),
        token_info({ { 3, 37 }, { 3, 71 } }, hl_scopes::remark),
        token_info({ { 3, 72 }, { 3, 80 } }, hl_scopes::ignored),
        token_info({ { 4, 18 }, { 4, 31 } }, hl_scopes::instruction),
        token_info({ { 4, 31 }, { 4, 71 } }, hl_scopes::operand),
        token_info({ { 4, 71 }, { 4, 72 } }, hl_scopes::continuation),
        token_info({ { 4, 72 }, { 4, 80 } }, hl_scopes::ignored),
        token_info({ { 5, 15 }, { 5, 71 } }, hl_scopes::operand),
        token_info({ { 5, 71 }, { 5, 72 } }, hl_scopes::continuation),
        token_info({ { 6, 15 }, { 6, 23 } }, hl_scopes::operand),
        token_info({ { 6, 23 }, { 6, 71 } }, hl_scopes::remark),
        token_info({ { 6, 71 }, { 6, 72 } }, hl_scopes::continuation),
        token_info({ { 6, 72 }, { 6, 80 } }, hl_scopes::ignored),
        token_info({ { 7, 15 }, { 7, 71 } }, hl_scopes::operand),
        token_info({ { 7, 71 }, { 7, 72 } }, hl_scopes::continuation),
        token_info({ { 8, 15 }, { 8, 71 } }, hl_scopes::operand),
        token_info({ { 8, 71 }, { 8, 72 } }, hl_scopes::continuation),
        token_info({ { 9, 15 }, { 9, 31 } }, hl_scopes::operand),
        token_info({ { 10, 0 }, { 10, 1 } }, hl_scopes::label),
        token_info({ { 10, 2 }, { 10, 11 } }, hl_scopes::instruction),
        token_info({ { 10, 11 }, { 10, 71 } }, hl_scopes::operand),
        /* TODO - Missing recognition of remarks in the SQL  TYPE   IS statement
        token_info({ { 10, 14 }, { 10, 46 } }, hl_scopes::operand),
        token_info({ { 10, 46 }, { 10, 62 } }, hl_scopes::remark),
        */
        token_info({ { 10, 72 }, { 10, 75 } }, hl_scopes::ignored),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, db2_preprocessor_statement_reinclude)
{
    mock_parse_lib_provider libs({
        { "MEMBER", "  SQL TYPE IS RESULT_SET_LOCATOR VARYING" },
    });

    const std::string contents = "ABCDE EXEC  SQL   INCLUDE  MEMBER";

    analyzer a(contents,
        analyzer_options { source_file_loc, &libs, db2_preprocessor_options(), collect_highlighting_info::yes });
    a.analyze();

    const auto& tokens = a.take_semantic_tokens();
    const semantics::lines_info expected = {
        token_info({ { 0, 0 }, { 0, 5 } }, hl_scopes::label),
        token_info({ { 0, 6 }, { 0, 15 } }, hl_scopes::instruction),
        token_info({ { 0, 15 }, { 0, 33 } }, hl_scopes::operand),
    };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, nested_preprocessors)
{
    mock_parse_lib_provider libs({
        { "MEMBER", "A   EXEC CICS ABEND ABCODE('1234')" },
    });

    const std::string contents = "-INC  MEMBER";

    analyzer a(contents,
        analyzer_options { source_file_loc,
            &libs,
            std::vector<preprocessor_options> { endevor_preprocessor_options(), cics_preprocessor_options() },
            collect_highlighting_info::yes });
    a.analyze();

    const auto& tokens = a.take_semantic_tokens();
    const semantics::lines_info expected = {
        token_info({ { 0, 0 }, { 0, 4 } }, hl_scopes::instruction),
        token_info({ { 0, 6 }, { 0, 12 } }, hl_scopes::operand),
    };

    EXPECT_EQ(tokens, expected);
}
