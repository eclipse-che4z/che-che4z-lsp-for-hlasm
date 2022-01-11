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

#include <unordered_map>

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"

// test cics preprocessor emulator

using namespace hlasm_plugin::parser_library::processing;

namespace hlasm_plugin::parser_library::processing::test {
cics_preprocessor_options test_cics_current_options(const preprocessor& p);
std::pair<int, std::string> test_cics_miniparser(const std::vector<std::string_view>& list);
} // namespace hlasm_plugin::parser_library::processing::test

TEST(cics_preprocessor, asm_xopts_parsing)
{
    for (const auto [text_template, expected] :
        std::initializer_list<std::pair<std::string_view, cics_preprocessor_options>> {
            { " ", cics_preprocessor_options() },
            { "*ASM XOPTS(NOPROLOG)", cics_preprocessor_options(false) },
            { "*ASM XOPTS(NOEPILOG) ", cics_preprocessor_options(true, false) },
            { "*ASM XOPTS(NOEPILOG,NOPROLOG)", cics_preprocessor_options(false, false) },
            { "*ASM XOPTS(EPILOG,NOPROLOG)   ", cics_preprocessor_options(false, true) },
            { "*ASM XOPTS(NOEPILOG,NOPROLOG,LEASM) ", cics_preprocessor_options(false, false, true) },
            { "*ASM XOPTS(NOLEASM,NOEPILOG,NOPROLOG)", cics_preprocessor_options(false, false, false) },
            { "*ASM XOPTS(NOLEASM,NOEPILOG NOPROLOG)", cics_preprocessor_options(false, false, false) },
            { "*ASM XOPTS'NOLEASM,NOEPILOG NOPROLOG'", cics_preprocessor_options(false, false, false) },
            { "*ASM XOPTS(NOLEASM,NOEPILOG NOPROLOG'", cics_preprocessor_options(false, false, false) },
            { "*ASM XOPTS(SP)", cics_preprocessor_options() },
        })
    {
        auto p = preprocessor::create(
            cics_preprocessor_options {}, [](std::string_view) { return std::nullopt; }, nullptr);
        size_t lineno = 0;

        auto text = text_template;
        auto result = p->generate_replacement(text, lineno);
        EXPECT_FALSE(result.has_value());

        using hlasm_plugin::parser_library::processing::test::test_cics_current_options;
        EXPECT_EQ(test_cics_current_options(*p), expected) << text_template;
    }
}

struct cics_preprocessor_tests_basics_data
    : std::pair<std::pair<std::string_view, cics_preprocessor_options>, std::vector<std::string_view>>
{};

class cics_preprocessor_tests : public ::testing::TestWithParam<cics_preprocessor_tests_basics_data>
{};

std::ostream& operator<<(std::ostream& os, const cics_preprocessor_tests_basics_data& v)
{
    auto line = v.first.first;
    auto result = std::string(lexing::extract_line(line).first);
    for (auto& c : result)
    {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
            c = '_';
    }
    return os << result;
}

TEST_P(cics_preprocessor_tests, basics)
{
    const auto& [input, expected] = GetParam();
    auto [text_template, config] = input;
    auto p = preprocessor::create(
        config, [](std::string_view) { return std::nullopt; }, nullptr);
    size_t lineno = 0;

    auto text = text_template;

    auto result_it = expected.begin();

    bool passed_empty_to_preprocessor = false;

    while (!passed_empty_to_preprocessor || !text.empty() || !p->finished())
    {
        if (text.empty())
            passed_empty_to_preprocessor = true;

        auto result = p->generate_replacement(text, lineno);
        if (result.has_value())
        {
            std::string_view to_check = result.value();
            while (!to_check.empty())
            {
                ASSERT_NE(result_it, expected.end()) << text_template;
                EXPECT_EQ(lexing::extract_line(to_check).first, *result_it);
                ++result_it;
            }
        }
        else
        {
            if (text.empty())
                break;
            ASSERT_NE(result_it, expected.end()) << text_template;
            EXPECT_EQ(lexing::extract_line(text).first, *result_it);
            ++result_it;
        }
    }
    EXPECT_EQ(result_it, expected.end());
}

INSTANTIATE_TEST_SUITE_P(cics_preprocessor,
    cics_preprocessor_tests,
    ::testing::ValuesIn(std::initializer_list<cics_preprocessor_tests_basics_data> {
        { {
            { "A START", cics_preprocessor_options() },
            {
                "         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR",
                "A START",
                "         DFHEIENT                  INSERTED BY TRANSLATOR",
                "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.",
                "         DFHEIMSG 4",
            },
        } },
        { {
            { "A RSECT\n END", cics_preprocessor_options(true, false, true) },
            {
                "         DFHEIGBL ,,RS,LE          INSERTED BY TRANSLATOR",
                "A RSECT",
                "         DFHEIENT                  INSERTED BY TRANSLATOR",
                "         DFHEISTG                  INSERTED BY TRANSLATOR",
                "         DFHEIEND                  INSERTED BY TRANSLATOR",
                " END",
            },
        } },
        { {
            { "A CSECT\n END", cics_preprocessor_options(false, true, true) },
            {
                "         DFHEIGBL ,,,LE            INSERTED BY TRANSLATOR",
                "A CSECT",
                "         DFHEIRET                  INSERTED BY TRANSLATOR",
                " END",
            },
        } },
        { {
            { "ABC    EXEC CICS ABEND ABCODE('1234') NODUMP\n END\n", cics_preprocessor_options() },
            {
                "*      EXEC CICS ABEND ABCODE('1234') NODUMP",
                "ABC      DFHECALL =X'0E'",
                "         DFHEIRET                  INSERTED BY TRANSLATOR",
                "         DFHEISTG                  INSERTED BY TRANSLATOR",
                "         DFHEIEND                  INSERTED BY TRANSLATOR",
                " END",
            },
        } },
        { {
            { " LA 0,DFHRESP(NORMAL)\n", cics_preprocessor_options() },
            {
                "*LA 0,DFHRESP(NORMAL)",
                "         LA   0,=F'0'",
                "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.",
                "         DFHEIMSG 4",
            },
        } },
        { {
            { "A LA 0,DFHRESP(NORMAL)\n", cics_preprocessor_options() },
            {
                "* LA 0,DFHRESP(NORMAL)",
                "A        LA   0,=F'0'",
                "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.",
                "         DFHEIMSG 4",
            },
        } },
        { {
            { " LA 0,DFHRESP()\n", cics_preprocessor_options() },
            {
                " LA 0,DFHRESP()",
                "*DFH7218I S  SUB-OPERAND(S) OF 'DFHRESP' CANNOT BE NULL. COMMAND NOT",
                "*            TRANSLATED.",
                "         DFHEIMSG 12",
                "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.",
                "         DFHEIMSG 4",
            },
        } },
    }));

class cics_preprocessor_dfhresp_fixture
    : public ::testing::TestWithParam<std::pair<std::vector<std::string_view>, std::pair<int, std::string>>>
{};

TEST_P(cics_preprocessor_dfhresp_fixture, dfhresp_substitution)
{
    using hlasm_plugin::parser_library::processing::test::test_cics_miniparser;

    auto [input, expected] = GetParam();

    EXPECT_EQ(test_cics_miniparser(input), expected);
}

INSTANTIATE_TEST_SUITE_P(cics_preprocessor,
    cics_preprocessor_dfhresp_fixture,
    ::testing::ValuesIn(std::initializer_list<std::pair<std::vector<std::string_view>, std::pair<int, std::string>>> {
        {
            {},
            { 0, "" },
        },
        {
            { "" },
            { 0, "" },
        },
        {
            { "AAAA" },
            { 0, "AAAA" },
        },
        {
            {
                "AAAA",
                "BBBB",
            },
            { 0, "AAAABBBB" },
        },
        {
            {
                "AAAA ",
                "BBBB",
            },
            { 0, "AAAA" },
        },
        {
            { "DFHRESP()" },
            { -1, "" },
        },
        {
            { "A,DFHRESP()" },
            { -1, "" },
        },
        {
            {
                "A ",
                "DFHRESP()",
            },
            { 0, "A" },
        },
        {
            { "A,", "DFHRESP()" },
            { -1, "" },
        },
        {
            { "DFHRESP(NORMAL)" },
            { 1, "=F'0'" },
        },
        {
            { "DFHRESP(NORMAL),DFHRESP(NORMAL)" },
            { 2, "=F'0',=F'0'" },
        },
        {
            { "DFHRESP(NORMAL) DFHRESP(NORMAL)" },
            { 1, "=F'0'" },
        },
        {
            {
                "DFHRESP(NORMAL) ",
                "DFHRESP(NORMAL)",
            },
            { 1, "=F'0'" },
        },
        {
            {
                "DFHRESP(NORMAL),",
                "DFHRESP(NORMAL)",
            },
            { 2, "=F'0',=F'0'" },
        },
        {
            {
                "DFHRESP(NORMAL), DFHRESP(NORMAL)",
                "DFHRESP(NORMAL)",
            },
            { 2, "=F'0',=F'0'" },
        },
        {
            { "L'DFHRESP(NORMAL)" },
            { 0, "L'DFHRESP(NORMAL)" },
        },
        {
            {
                "L'DFHRESP(NORMAL),",
                "DFHRESP(NORMAL)",
            },
            { 1, "L'DFHRESP(NORMAL),=F'0'" },
        },
        {
            { "L'DFHRESP(DFHRESP(NORMAL))" },
            { 1, "L'DFHRESP(=F'0')" },
        },
        {
            { "=C'DFHRESP(NORMAL)'" },
            { 0, "=C'DFHRESP(NORMAL)'" },
        },
        {
            { "L'DFHRESP(NORMAL),=C'DFHRESP(NORMAL)'" },
            { 0, "L'DFHRESP(NORMAL),=C'DFHRESP(NORMAL)'" },
        },
        {
            { "DFHRESP(ERROR),=C'DFHRESP(NORMAL)',1+DFHRESP(INVREQ)" },
            { 2, "=F'1',=C'DFHRESP(NORMAL)',1+=F'16'" },
        },
        {
            { "DFhreSP( ERROR),1+dFHRESP ( INvrEQ )" },
            { 2, "=F'1',1+=F'16'" },
        },
        {
            { "=C'DFHRESP(NORMAL)" }, // invalid code
            { 0, "=C'DFHRESP(NORMAL)" },
        },
        {
            { "A.DFHRESP(NORMAL)" },
            { 1, "A.=F'0'" },
        },
        {
            { "ADFHRESP(NORMAL)" },
            { 0, "ADFHRESP(NORMAL)" },
        },
        {
            { "DFHRESPZ(NORMAL)" },
            { 0, "DFHRESPZ(NORMAL)" },
        },
        {
            { "ADFHRESPZ(NORMAL)" },
            { 0, "ADFHRESPZ(NORMAL)" },
        },
    }));

TEST(cics_preprocessor, check_continuation_error_message)
{
    std::string input = R"(
         MACRO
         DFHECALL
         MEND
         MACRO
         DFHEIMSG
         MEND

         EXEC CICS ABEND ABCODE('1234') NODUMP                         X
AAAAAA   SAM31
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CIC001" }));
}

TEST(cics_preprocessor, check_null_argument_message)
{
    std::string input = R"(
         MACRO
         DFHEIMSG
         MEND

         LARL 0,DFHRESP()
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(contains_message_codes(a.diags(), { "CIC002" }));
}

TEST(cics_preprocessor, dfhresp_substitution)
{
    std::string input = R"(
         LARL 0,DFHRESP(NORMAL)
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();
    a.collect_diags();

    EXPECT_TRUE(a.diags().empty());
}
