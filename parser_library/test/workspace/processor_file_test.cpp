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
#include "files_parse_lib_provider.h"
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
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(mngr, config, global_settings);
    ws.open();
    auto file = ws.add_processor_file(file_loc);

    // Prior to parsing, there is no lsp_context available

    const auto* fp = file->get_lsp_context();
    ASSERT_FALSE(fp);
}

TEST(processor_file, parse_macro)
{
    resource_location opencode_loc("filename");
    resource_location macro_loc("MAC");

    file_manager_impl mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(mngr, config, global_settings);
    ws.open();
    files_parse_lib_provider provider(mngr, ws);

    mngr.did_open_file(opencode_loc, 0, " SAM31\n MAC");
    auto opencode = ws.add_processor_file(opencode_loc);

    mngr.did_open_file(macro_loc, 0, R"( MACRO
 MAC
 SAM31
 MEND)");
    auto macro = ws.add_processor_file(macro_loc);

    opencode->parse(provider, {}, {}, nullptr);

    // Opencode file tests
    const auto* open_fp = opencode->get_lsp_context();
    ASSERT_TRUE(open_fp);

    EXPECT_EQ(open_fp->definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));

    const std::string sam31_hover_message = "**Set Addressing Mode (31)**\n\nMachine instruction, format: E\n\n"
                                            "Operands: \n\nCondition Code: The code remains unchanged\n\nDetails on "
                                            "[page 885](https://publibfp.dhe.ibm.com/epubs/pdf/a227832d.pdf#page=885 "
                                            "\"Principles of Operations (SA22-7832-13)\")";
    EXPECT_EQ(open_fp->hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(open_fp->hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info open_expected_hl {
        { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction },
    };

    EXPECT_EQ(opencode->get_hl_info(), open_expected_hl);

    performance_metrics expected_metrics;
    expected_metrics.files = 2;
    expected_metrics.lines = 6;
    expected_metrics.macro_def_statements = 4;
    expected_metrics.macro_statements = 2;
    expected_metrics.non_continued_statements = 6;
    expected_metrics.open_code_statements = 2;
    EXPECT_EQ(opencode->get_metrics(), expected_metrics);


    // Macro file tests
    const auto* macro_fp = macro->get_lsp_context();
    ASSERT_TRUE(macro_fp);
    EXPECT_EQ(macro_fp->definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));
    EXPECT_EQ(macro_fp->hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(macro_fp->hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info macro_expected_hl {
        { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction },
        { 2, 1, 2, 6, semantics::hl_scopes::instruction },
        { 3, 1, 3, 5, semantics::hl_scopes::instruction },
    };

    EXPECT_EQ(macro->get_hl_info(), macro_expected_hl);
    EXPECT_EQ(macro->get_metrics(), expected_metrics);
}
