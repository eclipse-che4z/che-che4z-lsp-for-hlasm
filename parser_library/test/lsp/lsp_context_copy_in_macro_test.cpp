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

#include "../compare_unordered_vectors.h"
#include "analyzer_fixture.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_copy_in_macro : public analyzer_fixture
{
    const static inline std::string opencode =
        R"(
       MAC

       COPY COPYFILE
       LR &VAR,1
       L 1,SYM
)";
    const static inline std::string macro_file_name = "MAC";
    const static inline std::string macro =
        R"( MACRO
       MAC &PARAM

       COPY COPYFILE

 SYM   LR &VAR,1

       MEND
)";
    const static inline std::string copyfile_file_name = "COPYFILE";
    const static inline std::string copyfile =
        R"(
       
       LR &PARAM,1
&VAR   SETA 1

)";
    class lib_provider_mock : public workspaces::parse_lib_provider
    {
        asm_option empty_options;
        workspaces::parse_result parse_library(
            const std::string& library, analyzing_context ctx, const workspaces::library_data data) override
        {
            const std::string* text;
            if (library == copyfile_file_name)
                text = &copyfile;
            else if (library == macro_file_name)
                text = &macro;
            else
                return false;

            analyzer lib_analyzer(*text, library, ctx, *this, data);
            lib_analyzer.analyze();
            return true;
        };

        bool has_library(const std::string& library, const std::string& program) const override
        {
            return library == copyfile_file_name || library == macro_file_name;
        };

        const asm_option& get_asm_options(const std::string&) override { return empty_options; };
    };
    static inline lib_provider_mock lib_prov_instance;
    lsp_context_copy_in_macro()
        : analyzer_fixture(opencode, lib_prov_instance)
    {}
};

TEST_F(lsp_context_copy_in_macro, definition_macro)
{
    location res = a.context().lsp_ctx->definition(opencode_file_name, { 1, 8 });
    EXPECT_EQ(res.file, macro_file_name);
    EXPECT_EQ(res.pos, position(1, 7));
}