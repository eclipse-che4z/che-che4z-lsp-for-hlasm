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

#include <initializer_list>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../common_testing.h"
#include "../mock_parse_lib_provider.h"
#include "analyzer.h"
#include "lexing/logical_line.h"
#include "preprocessor_options.h"
#include "processing/preprocessor.h"
#include "semantics/source_info_processor.h"

// test cics preprocessor emulator

using namespace hlasm_plugin::parser_library::processing;

namespace hlasm_plugin::parser_library::processing::test {
cics_preprocessor_options test_cics_current_options(const preprocessor& p);
std::pair<int, std::string> test_cics_miniparser(const std::vector<std::string_view>& list);
} // namespace hlasm_plugin::parser_library::processing::test

constexpr auto empty_library_fetcher =
    [](std::string) -> hlasm_plugin::utils::value_task<
                        std::optional<std::pair<std::string, hlasm_plugin::utils::resource::resource_location>>> {
    co_return std::nullopt;
};

TEST(cics_preprocessor, asm_xopts_parsing)
{
    for (const auto& [text_template, expected] :
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
        semantics::source_info_processor src_info(false);

        auto p = preprocessor::create(cics_preprocessor_options {}, empty_library_fetcher, nullptr, src_info);

        auto result = p->generate_replacement(document(text_template)).run().value();
        EXPECT_GT(result.size(), 0);

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
    semantics::source_info_processor src_info(false);
    const auto& [input, expected] = GetParam();
    auto [text_template, config] = input;

    auto p = preprocessor::create(config, empty_library_fetcher, nullptr, src_info);

    auto result = p->generate_replacement(document(text_template)).run().value();

    EXPECT_TRUE(std::ranges::equal(expected, result, {}, {}, [](const auto& e) {
        auto text = e.text();
        while (!text.empty() && (text.back() == '\n' || text.back() == '\r'))
            text.remove_suffix(1);
        return text;
    })) << text_template;
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
            { " LA 0,DFHVALUE(FIRSTQUIESCE)\n", cics_preprocessor_options() },
            {
                "*LA 0,DFHVALUE(FIRSTQUIESCE)",
                "         LA   0,=F'182'",
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
        { {
            { " LA 0,DFHVALUE()\n", cics_preprocessor_options() },
            {
                " LA 0,DFHVALUE()",
                "*DFH7218I S  SUB-OPERAND(S) OF 'DFHVALUE' CANNOT BE NULL. COMMAND NOT",
                "*            TRANSLATED.",
                "         DFHEIMSG 12",
                "*DFH7041I W  NO END CARD FOUND - COPYBOOK ASSUMED.",
                "         DFHEIMSG 4",
            },
        } },
        { {
            { "DFHEISTG DSECT\n END", cics_preprocessor_options(false, false, false) },
            {
                "         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR",
                "DFHEISTG DSECT",
                " END",
            },
        } },
        { {
            { "DFHEISTG DSECT\n END", cics_preprocessor_options(true, false, false) },
            {
                "         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR",
                "DFHEISTG DSECT",
                "         DFHEISTG                  INSERTED BY TRANSLATOR",
                "         DFHEISTG                  INSERTED BY TRANSLATOR",
                "         DFHEIEND                  INSERTED BY TRANSLATOR",
                " END",
            },
        } },
        { {
            { "*PROCESS OVERRIDE(RENT)\n*ASM XOPTS(NOPROLOG,NOEPILOG)\n END", cics_preprocessor_options() },
            {
                "*PROCESS OVERRIDE(RENT)",
                "*ASM XOPTS(NOPROLOG,NOEPILOG)",
                " END",
            },
        } },
        { {
            { "*PROCESS OVERRIDE(RENT)\n*ASM cics(NOPROLOG,NOEPILOG)\n END", cics_preprocessor_options() },
            {
                "*PROCESS OVERRIDE(RENT)",
                "*ASM cics(NOPROLOG,NOEPILOG)",
                " END",
            },
        } },
        { {
            { "         DFHEISTG\n END", cics_preprocessor_options(false, false, false) },
            {
                "         DFHEIGBL ,,,NOLE          INSERTED BY TRANSLATOR",
                "         DFHEISTG",
                " END",
            },
        } },
        { {
            { "*ASM XOPT(NOPROLOG NOEPILOG) ", cics_preprocessor_options() },
            {
                "*ASM XOPT(NOPROLOG NOEPILOG) ",
            },
        } },
        { {
            { "*ASM XOPT(NOPROLOG NOEPILOG) \n END", cics_preprocessor_options() },
            {
                "*ASM XOPT(NOPROLOG NOEPILOG) ",
                " END",
            },
        } },
    }));

class cics_preprocessor_dfh_fixture
    : public ::testing::TestWithParam<std::pair<std::vector<std::string_view>, std::pair<int, std::string>>>
{};

TEST_P(cics_preprocessor_dfh_fixture, dfh_substitution)
{
    using hlasm_plugin::parser_library::processing::test::test_cics_miniparser;

    auto [input, expected] = GetParam();

    EXPECT_EQ(test_cics_miniparser(input), expected);
}

INSTANTIATE_TEST_SUITE_P(cics_preprocessor,
    cics_preprocessor_dfh_fixture,
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
            { "DFHVALUE()" },
            { -1, "" },
        },
        {
            { "A,DFHVALUE()" },
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
            { "DFHVALUE(FIRSTQUIESCE)" },
            { 1, "=F'182'" },
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
            { "L'DFHVALUE(DFHRESP(NORMAL))" },
            { 1, "L'DFHVALUE(=F'0')" },
        },
        {
            { "L'DFHRESP(DFHVALUE(FIRSTQUIESCE))" },
            { 1, "L'DFHRESP(=F'182')" },
        },
        {
            { "=C'DFHRESP(NORMAL)'" },
            { 0, "=C'DFHRESP(NORMAL)'" },
        },
        {
            { "=C'DFHVALUE(FIRSTQUIESCE)'" },
            { 0, "=C'DFHVALUE(FIRSTQUIESCE)'" },
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
        {
            { "A.DFHVALUE(FIRSTQUIESCE)" },
            { 1, "A.=F'182'" },
        },
        {
            { "ADFHVALUE(NORMAL)" },
            { 0, "ADFHVALUE(NORMAL)" },
        },
        {
            { "DFHVALUEZ(NORMAL)" },
            { 0, "DFHVALUEZ(NORMAL)" },
        },
        {
            { "ADFHVALUEZ(NORMAL)" },
            { 0, "ADFHVALUEZ(NORMAL)" },
        },
        {
            {
                "DFHRESP(NORMAL),",
                "DFHVALUE(FIRSTQUIESCE)",
            },
            { 2, "=F'0',=F'182'" },
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

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CIC001" }));
}

TEST(cics_preprocessor, check_missing_command)
{
    const std::string input = R"(
         MACRO
         DFHECALL
         MEND
         MACRO
         DFHEIMSG
         MEND

SPACE    EXEC CICS 
NOSPACE  EXEC CICS)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "CIC003", "CIC003" }));
}

TEST(cics_preprocessor, check_null_argument_message)
{
    std::string input = R"(
         MACRO
         DFHEIMSG
         MEND

         LARL 0,DFHRESP()
         LARL 0,DFHVALUE()
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();

    EXPECT_TRUE(contains_message_codes(a.diags(), { "CIC002", "CIC002" }));
}

TEST(cics_preprocessor, dfhresp_substitution)
{
    std::string input = R"(
         LARL 0,DFHRESP(NORMAL)
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(cics_preprocessor, dfhvalue_substitution)
{
    std::string input = R"(
         LARL 0,DFHVALUE(FIRSTQUIESCE)
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}

TEST(cics_preprocessor, substitution_in_macros)
{
    std::string input = R"(
         MACRO
         MAC  &P1
         LARL 0,&P1(1)
         LARL 0,&P1(2)
         MEND
         MAC  (DFHRESP(NORMAL),DFHVALUE(FIRSTQUIESCE))
         END
)";
    analyzer a(input, analyzer_options(cics_preprocessor_options(false, false, false)));

    a.analyze();

    EXPECT_TRUE(a.diags().empty());
}
