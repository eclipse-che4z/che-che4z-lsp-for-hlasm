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

namespace hlasm_plugin::parser_library::processing {
cics_preprocessor_options test_cics_current_options(const preprocessor& p);
}

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

    while (!text.empty() || !p->finished())
    {
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
    }));
