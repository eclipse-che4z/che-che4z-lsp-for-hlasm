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

#include "analyzer_fixture.h"
#include "lsp/item_convertors.h"
#include "lsp/lsp_context.h"
#include "lsp_context_test_helper.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::lsp;

struct lsp_context_macro_documentation : public analyzer_fixture
{
    const static inline std::string input =
        R"(
* Before macro line 1
* Before macro line 2
       MACRO
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
* After macro line 1
.* After macro line 2
       MEND

       MAC
 
)";

    const static inline std::string macro_documentation = R"(```
       MAC     &FIRST_PARAM,      first param remark                   X
               &SECOND_PARAM=1    second param remark
* Before macro line 1
* Before macro line 2
* After macro line 1
.* After macro line 2

```
)";


    lsp_context_macro_documentation()
        : analyzer_fixture(input)
    {}
};


TEST_F(lsp_context_macro_documentation, definition)
{
    location res = a.context().lsp_ctx->definition(opencode_loc, { 10, 8 });
    check_location_with_position(res, opencode_loc, 4, 7);
}

TEST_F(lsp_context_macro_documentation, references)
{
    auto res = a.context().lsp_ctx->references(opencode_loc, { 10, 8 });
    ASSERT_EQ(res.size(), 2U);

    check_location_with_position(res[0], opencode_loc, 4, 7);
    check_location_with_position(res[1], opencode_loc, 10, 7);
}

TEST_F(lsp_context_macro_documentation, hover)
{
    auto res = a.context().lsp_ctx->hover(opencode_loc, { 10, 8 });

    EXPECT_EQ(res, macro_documentation);
}

TEST_F(lsp_context_macro_documentation, completion)
{
    auto res_v = a.context().lsp_ctx->completion(opencode_loc, { 11, 1 }, '\0', completion_trigger_kind::invoked);

    ASSERT_TRUE(std::holds_alternative<completion_list_instructions>(res_v));

    const auto& res = std::get<completion_list_instructions>(res_v);

    ASSERT_TRUE(res.macros);
    EXPECT_TRUE(std::any_of(
        res.macros->begin(), res.macros->end(), [](const auto& m) { return m.first->id.to_string_view() == "MAC"; }));
}

TEST(lsp_context_macro_documentation_incomplete, incomplete_macro)
{
    auto file_loc = hlasm_plugin::utils::resource::resource_location("source");
    std::string input = R"( 
 MACRO
 )";
    analyzer a(input, analyzer_options { file_loc });
    a.analyze();
    auto res_v = a.context().lsp_ctx->completion(file_loc, { 0, 1 }, '\0', completion_trigger_kind::invoked);

    EXPECT_TRUE(std::holds_alternative<completion_list_instructions>(res_v));
}
