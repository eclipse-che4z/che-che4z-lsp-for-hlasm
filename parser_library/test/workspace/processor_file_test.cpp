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

#include "gtest/gtest.h"

#include "../gtest_stringers.h"
#include "../workspace/empty_configs.h"
#include "library_mock.h"
#include "lsp/lsp_context.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/processor_file_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

TEST(processor_file, no_lsp_context)
{
    resource_location file_loc("filename");
    file_manager_impl mngr;
    mngr.did_open_file(file_loc, 0, " LR 1,1");

    processor_file_impl file(mngr.find(file_loc), mngr);

    // Prior to parsing, there is no lsp_context available

    const auto* fp = file.get_lsp_context();
    ASSERT_FALSE(fp);
}

TEST(processor_file, parse_macro)
{
    resource_location opencode_loc("filename");
    resource_location macro_loc("MAC");

    file_manager_impl mngr;

    mngr.did_open_file(opencode_loc, 0, " SAM31\n MAC");

    mngr.did_open_file(macro_loc, 0, R"( MACRO
 MAC
 SAM31
 MEND)");

    using namespace ::testing;
    shared_json global_settings = make_empty_shared_json();
    lib_config config;
    auto library = std::make_shared<NiceMock<library_mock>>();
    workspace ws(mngr, config, global_settings, nullptr, library);

    EXPECT_CALL(*library, has_file(std::string_view("MAC"), _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(macro_loc), Return(true)));

    ws.did_open_file(opencode_loc, open_file_result::changed_content);

    // Opencode file tests

    EXPECT_EQ(ws.definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));

    const std::string sam31_hover_message = "**Set Addressing Mode (31)**\n\nMachine instruction, format: E\n\n"
                                            "Operands: \n\nCondition Code: The code remains unchanged\n\nDetails on "
                                            "[page 885](https://publibfp.dhe.ibm.com/epubs/pdf/a227832d.pdf#page=885 "
                                            "\"Principles of Operations (SA22-7832-13)\")";
    EXPECT_EQ(ws.hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(ws.hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info open_expected_hl {
        { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction },
    };

    EXPECT_EQ(ws.semantic_tokens(opencode_loc), open_expected_hl);

    performance_metrics expected_metrics;
    expected_metrics.files = 2;
    expected_metrics.lines = 6;
    expected_metrics.macro_def_statements = 4;
    expected_metrics.macro_statements = 2;
    expected_metrics.non_continued_statements = 6;
    expected_metrics.open_code_statements = 2;
    EXPECT_EQ(ws.find_processor_file(opencode_loc)->get_metrics(), expected_metrics);

    // Macro file tests
    semantics::lines_info macro_expected_hl {
        { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction },
        { 2, 1, 2, 6, semantics::hl_scopes::instruction },
        { 3, 1, 3, 5, semantics::hl_scopes::instruction },
    };

    EXPECT_EQ(ws.semantic_tokens(macro_loc), macro_expected_hl);
}
