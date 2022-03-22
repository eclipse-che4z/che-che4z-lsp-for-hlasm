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

#include "../gtest_stringers.h"
#include "analyzer.h"
#include "workspaces/parse_lib_provider.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::semantics;

namespace std {

inline void PrintTo(const lines_info& tokens, std::ostream* os)
{
    for (const token_info& t : tokens)
    {
        *os << "{ " << t.token_range << ", " << (std::underlying_type_t<hl_scopes>)t.scope << " },\n";
    }
}
} // namespace std

TEST(highlighting, simple)
{
    std::string source_file = "file_name";
    const std::string contents = "A EQU 1";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 0 }, { 0, 1 } }, hl_scopes::label),
        token_info({ { 0, 2 }, { 0, 5 } }, hl_scopes::instruction),
        token_info({ { 0, 6 }, { 0, 7 } }, hl_scopes::number) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_expr)
{
    std::string source_file = "file_name";
    const std::string contents = " LR 1*1+X,L'X";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = " L X'F',*";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 2 } }, hl_scopes::instruction),
        token_info({ { 0, 3 }, { 0, 4 } }, hl_scopes::self_def_type),
        token_info({ { 0, 4 }, { 0, 7 } }, hl_scopes::string),
        token_info({ { 0, 7 }, { 0, 8 } }, hl_scopes::operator_symbol),
        token_info({ { 0, 8 }, { 0, 9 } }, hl_scopes::operand) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, mach_expr_3)
{
    std::string source_file = "file_name";
    const std::string contents = " L 1,=C'1'";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = " DC 4CAP8L4''";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = " AMODE ANY64";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
    semantics::lines_info expected = { token_info({ { 0, 1 }, { 0, 6 } }, hl_scopes::instruction),
        token_info({ { 0, 7 }, { 0, 12 } }, hl_scopes::ordinary_symbol) };

    EXPECT_EQ(tokens, expected);
}

TEST(highlighting, asm_list)
{
    std::string source_file = "file_name";
    const std::string contents = " AMODE (op2,op3)";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = " AMODE op1(op2,op3)";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents =
        R"(D EQU                                                                 1Xignored
IgnoredIgnoredI1 remark)";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = R"(
 MACRO
 MAC
 MEND
 MAC OP1, remark                                                       X
               OP2 remark2)";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
    semantics::lines_info expected = { token_info({ { 1, 1 }, { 1, 6 } }, hl_scopes::instruction),
        token_info({ { 2, 1 }, { 2, 4 } }, hl_scopes::instruction),
        token_info({ { 3, 1 }, { 3, 5 } }, hl_scopes::instruction),
        token_info({ { 4, 1 }, { 4, 4 } }, hl_scopes::instruction),
        token_info({ { 4, 5 }, { 4, 8 } }, hl_scopes::operand),
        token_info({ { 4, 8 }, { 4, 71 } }, hl_scopes::operator_symbol),
        token_info({ { 4, 10 }, { 4, 71 } }, hl_scopes::remark),
        token_info({ { 4, 71 }, { 4, 72 } }, hl_scopes::continuation),
        token_info({ { 5, 0 }, { 5, 15 } }, hl_scopes::ignored),
        token_info({ { 5, 15 }, { 5, 18 } }, hl_scopes::operand),
        token_info({ { 5, 19 }, { 5, 26 } }, hl_scopes::remark) };

    EXPECT_EQ(tokens, expected);
}


TEST(highlighting, var_sym_array_subscript)
{
    std::string source_file = "file_name";
    const std::string contents = "&VARP(31+L'C) SETA 45\n\nC EQU 1";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
    const std::string contents = " AIF (T'&SYSDATC EQ 'C').LOOP";
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
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
    const auto& tokens = a.source_processor().semantic_tokens();
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
    std::string source_file = "file_name";
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
    analyzer a(contents, analyzer_options { source_file, collect_highlighting_info::yes });
    a.analyze();
    const auto& tokens = a.source_processor().semantic_tokens();
    semantics::lines_info expected;

    for (size_t i = 1; i <= 'Z' - 'A' + 1; ++i)
    {
        expected.emplace_back(token_info({ { i, 0 }, { i, 2 } }, hl_scopes::var_symbol));
        expected.emplace_back(token_info({ { i, 3 }, { i, 7 } }, hl_scopes::instruction));
        expected.emplace_back(token_info({ { i, 8 }, { i, 11 } }, hl_scopes::string));
    }

    EXPECT_EQ(tokens, expected);
}