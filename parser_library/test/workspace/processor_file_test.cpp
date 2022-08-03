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
#include "files_parse_lib_provider.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/processor_file_impl.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

TEST(processor_file, empty_file_feature_provider)
{
    resource_location file_loc("filename");
    file_manager_impl mngr;
    mngr.did_open_file(file_loc, 0, " LR 1,1");
    auto file = mngr.add_processor_file(file_loc);

    // Prior to parsing, it should return default values

    auto& fp = file->get_lsp_feature_provider();

    EXPECT_EQ(fp.definition(file_loc, { 0, 5 }), location({ 0, 5 }, file_loc));
    EXPECT_EQ(fp.references(file_loc, { 0, 5 }), location_list());
    EXPECT_EQ(fp.hover(file_loc, { 0, 5 }), "");
    EXPECT_EQ(fp.completion(file_loc, { 0, 5 }, '\0', completion_trigger_kind::invoked), lsp::completion_list_s());

    EXPECT_EQ(file->get_hl_info(), semantics::lines_info());
    EXPECT_EQ(file->get_metrics(), performance_metrics());
}

TEST(processor_file, parse_macro)
{
    resource_location opencode_loc("filename");
    resource_location macro_loc("MAC");

    file_manager_impl mngr;
    files_parse_lib_provider provider(mngr);

    mngr.did_open_file(opencode_loc, 0, " SAM31\n MAC");
    auto opencode = mngr.add_processor_file(opencode_loc);

    mngr.did_open_file(macro_loc, 0, R"( MACRO
 MAC
 SAM31
 MEND)");
    auto macro = mngr.add_processor_file(macro_loc);

    opencode->parse(provider, {}, {}, nullptr);

    // Opencode file tests
    auto& open_fp = opencode->get_lsp_feature_provider();

    EXPECT_EQ(open_fp.definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));

    const std::string sam31_hover_message = "Operands: \n\nMachine instruction \n\nInstruction format: E";
    EXPECT_EQ(open_fp.hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(open_fp.hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info open_expected_hl { { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction } };

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
    auto& macro_fp = macro->get_lsp_feature_provider();
    EXPECT_EQ(macro_fp.definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));
    EXPECT_EQ(macro_fp.hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(macro_fp.hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info macro_expected_hl { { 0, 1, 0, 6, semantics::hl_scopes::instruction },
        { 1, 1, 1, 4, semantics::hl_scopes::instruction },
        { 2, 1, 2, 6, semantics::hl_scopes::instruction },
        { 3, 1, 3, 5, semantics::hl_scopes::instruction } };

    EXPECT_EQ(macro->get_hl_info(), macro_expected_hl);
    EXPECT_EQ(macro->get_metrics(), expected_metrics);
}
