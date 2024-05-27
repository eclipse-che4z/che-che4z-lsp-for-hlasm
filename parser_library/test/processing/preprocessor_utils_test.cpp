/*
 * Copyright (c) 2022 Broadcom.
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


#include <string>
#include <string_view>
#include <vector>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "processing/preprocessors/preprocessor_utils.h"
#include "semantics/range_provider.h"
#include "semantics/statement.h"

namespace {
static constexpr const lexing::logical_line_extractor_args extract_opts { 1, 71, 2, false, false };

semantics::range_provider get_range_provider(size_t text_length, size_t lineno, size_t continue_column)
{
    return semantics::range_provider(range(position(lineno, 0), position(lineno, text_length)),
        semantics::adjusting_state::MACRO_REPARSE,
        continue_column);
}

std::string get_inline_string(std::string_view text, const lexing::logical_line_extractor_args& opts)
{
    lexing::logical_line<std::string_view::iterator> out;
    out.clear();

    while (true)
    {
        auto [next, it] = append_to_logical_line(out, text, opts);
        if (!next)
            break;
        text.remove_prefix(std::ranges::distance(text.begin(), it));
    }

    finish_logical_line(out, opts);

    return std::string(out.begin(), out.end());
}
} // namespace

TEST(preprocessor_utils, operand_parsing_single)
{
    std::string input = get_inline_string(R"(  ABCODE    )", extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE", range(position(1, 2), position(1, 8)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 1, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_single_argument)
{
    std::string input = get_inline_string(R"(ABCODE('1234')   )", extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE('1234')", range(position(0, 0), position(0, 14)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_single_op_column)
{
    std::string input = get_inline_string(R"(ABCODE    )", extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE", range(position(0, 14), position(0, 20)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 14, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_single_argument_multiline)
{
    std::string input = get_inline_string(R"(ABCODE('12                                                             X
        34' ))",
        extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE('1234')", range(position(0, 0), position(1, 13)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_multiple)
{
    std::string input = get_inline_string(R"(ABCODE ( '1234' ) NODUMP RECFM ( X'02' ) OPERAND ('4321'))", extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE('1234')", range(position(0, 0), position(0, 17)) },
        { "NODUMP", range(position(0, 18), position(0, 24)) },
        { "RECFM(X'02')", range(position(0, 25), position(0, 40)) },
        { "OPERAND('4321')", range(position(0, 41), position(0, 57)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_multiple_comma_separated)
{
    std::string input = get_inline_string(R"(1,2,3,DFHVALUE(ACQUIRED))", extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "1", range(position(0, 0), position(0, 1)) },
        { "2", range(position(0, 2), position(0, 3)) },
        { "3", range(position(0, 4), position(0, 5)) },
        { "DFHVALUE(ACQUIRED)", range(position(0, 6), position(0, 24)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_multiple_multiline)
{
    auto input = get_inline_string(
        R"(ABCODE ( '1234' )                                                      X
               NODUMP                                                  X
               OPERAND ('4321'))",
        extract_opts);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE('1234')", range(position(0, 0), position(0, 17)) },
        { "NODUMP", range(position(1, 15), position(1, 21)) },
        { "OPERAND('4321')", range(position(2, 15), position(2, 31)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 1)), expected);
}

TEST(preprocessor_utils, operand_parsing_multiple_multiline_continue)
{
    auto input = get_inline_string(
        R"(ABCODE ( '1234' )                                                      X
               NODUMP                                                  X
               OPERAND ('4321'))",
        lexing::default_ictl);

    std::vector<semantics::preproc_details::name_range> expected {
        { "ABCODE('1234')", range(position(0, 0), position(0, 17)) },
        { "NODUMP", range(position(1, 15), position(1, 21)) },
        { "OPERAND('4321')", range(position(2, 15), position(2, 31)) },
    };

    EXPECT_EQ(processing::get_operands_list(input, 0, get_range_provider(input.length(), 0, 15)), expected);
}
