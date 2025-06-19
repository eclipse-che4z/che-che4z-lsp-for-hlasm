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

#include "../common_testing.h"
#include "../gtest_stringers.h"
#include "../workspace/empty_configs.h"
#include "library_mock.h"
#include "lsp/lsp_context.h"
#include "semantics/highlighting_info.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

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
    resource_location lib_loc("");
    auto library = std::make_shared<NiceMock<library_mock>>();

    EXPECT_CALL(*library, get_location).WillOnce(ReturnRef(lib_loc));

    workspace_configuration ws_cfg(mngr, global_settings, config, library);
    workspace ws(mngr, ws_cfg);

    EXPECT_CALL(*library, has_file(std::string_view("MAC"), _))
        .WillRepeatedly(DoAll(SetArgPointee<1>(macro_loc), Return(true)));

    run_if_valid(ws.did_open_file(opencode_loc, file_content_state::changed_content));

    auto [url, wf_info, metrics, errors, warnings, outputs_changed] = ws.parse_file().run().value();
    EXPECT_EQ(url, opencode_loc);
    EXPECT_TRUE(metrics);

    // Opencode file tests

    EXPECT_EQ(ws.definition(opencode_loc, { 1, 2 }), location({ 1, 1 }, macro_loc));

    const std::string sam31_hover_message =
        "**Set Addressing Mode (31)**\n\nMachine instruction, format: E\n\n"
        "Operands: \n\nCondition Code: The code remains unchanged\n\nDetails on "
        "[page 950](https://www.ibm.com/docs/en/module_1678991624569/pdf/SA22-7832-14.pdf#page=950 "
        "\"Principles of Operations (SA22-7832-14)\")";
    EXPECT_EQ(ws.hover(opencode_loc, { 0, 2 }), sam31_hover_message);
    EXPECT_EQ(ws.hover(macro_loc, { 2, 2 }), sam31_hover_message);

    semantics::lines_info open_expected_hl {
        { 0, 1, 0, 6, hl_scopes::instruction },
        { 1, 1, 1, 4, hl_scopes::instruction },
    };

    EXPECT_EQ(ws.semantic_tokens(opencode_loc), open_expected_hl);

    performance_metrics expected_metrics;
    expected_metrics.lines = 6;
    expected_metrics.macro_def_statements = 4;
    expected_metrics.macro_statements = 2;
    expected_metrics.non_continued_statements = 6;
    expected_metrics.open_code_statements = 2;
    EXPECT_EQ(metrics, expected_metrics);
    EXPECT_EQ(ws.last_metrics(opencode_loc), expected_metrics);
    EXPECT_EQ(wf_info.files_processed, 2);

    // Macro file tests
    semantics::lines_info macro_expected_hl {
        { 0, 1, 0, 6, hl_scopes::instruction },
        { 1, 1, 1, 4, hl_scopes::instruction },
        { 2, 1, 2, 6, hl_scopes::instruction },
        { 3, 1, 3, 5, hl_scopes::instruction },
    };

    EXPECT_EQ(ws.semantic_tokens(macro_loc), macro_expected_hl);
}
