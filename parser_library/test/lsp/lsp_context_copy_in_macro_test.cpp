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

#include "../mock_parse_lib_provider.h"
#include "analyzer_fixture.h"
#include "lsp_context_test_helper.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;
using namespace hlasm_plugin::utils::resource;

struct lsp_context_copy_in_macro : public ::testing::Test
{
    const static inline std::string opencode_file_name = "source";
    const static inline auto opencode_file_loc = resource_location(opencode_file_name);
    const static inline std::string opencode =
        R"(
       MAC 1

       COPY COPYFILE
       LR &VAR,1
       L 1,SYM

       COPY NOTEXIST
)";
    const static inline std::string macro_file_name = "MAC";
    const static inline auto macro_file_loc = resource_location(macro_file_name);
    const static inline std::string macro =
        R"( MACRO
       MAC &PARAM

       COPY COPYFILE

SYM    LR &VAR,1

       MEND
)";
    const static inline std::string copyfile_file_name = "COPYFILE";
    const static inline resource_location copyfile_file_loc = resource_location(copyfile_file_name);
    const static inline std::string copyfile =
        R"(
       
       LR &PARAM,1
&VAR   SETA 1

)";

    mock_parse_lib_provider lib_prov_instance;
    std::unique_ptr<analyzer> a;
    lsp_context_copy_in_macro()
        : lib_prov_instance({ { macro_file_name, macro }, { copyfile_file_name, copyfile } })
    {}

    void SetUp() override
    {
        a = std::make_unique<analyzer>(opencode, analyzer_options { opencode_file_loc, &lib_prov_instance });
        a->analyze();
    }
};

TEST_F(lsp_context_copy_in_macro, definition_macro)
{
    location res = a->context().lsp_ctx->definition(opencode_file_loc, { 1, 8 });
    check_location_with_position(res, macro_file_loc, 1, 7);
}

TEST_F(lsp_context_copy_in_macro, definition_copyfile_from_opencode)
{
    location res = a->context().lsp_ctx->definition(opencode_file_loc, { 3, 13 });
    check_location_with_position(res, copyfile_file_loc, 0, 0);
}

TEST_F(lsp_context_copy_in_macro, definition_copyfile_from_macro)
{
    location res = a->context().lsp_ctx->definition(macro_file_loc, { 3, 13 });
    check_location_with_position(res, copyfile_file_loc, 0, 0);
}

TEST_F(lsp_context_copy_in_macro, definition_macro_param_from_copyfile)
{
    location res = a->context().lsp_ctx->definition(copyfile_file_loc, { 2, 11 });
    check_location_with_position(res, macro_file_loc, 1, 11);
}

TEST_F(lsp_context_copy_in_macro, definition_var_from_macro)
{
    location res = a->context().lsp_ctx->definition(macro_file_loc, { 5, 11 });
    check_location_with_position(res, copyfile_file_loc, 3, 0);
}

TEST_F(lsp_context_copy_in_macro, definition_var_from_opencode)
{
    location res = a->context().lsp_ctx->definition(opencode_file_loc, { 4, 11 });
    check_location_with_position(res, copyfile_file_loc, 3, 0);
}

TEST_F(lsp_context_copy_in_macro, definition_no_exist_copyfile)
{
    location res = a->context().lsp_ctx->definition(opencode_file_loc, { 7, 15 });
    check_location_with_position(res, opencode_file_loc, 7, 15);
}
