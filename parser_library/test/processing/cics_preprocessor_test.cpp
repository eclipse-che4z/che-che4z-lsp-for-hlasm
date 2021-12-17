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
