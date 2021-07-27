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

#include "workspaces/file_manager_impl.h"
#include "workspaces/processor_file_impl.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
TEST(processor_file, empty_file_feature_provider)
{
    std::string file_name = "filename";
    file_manager_impl mngr;
    mngr.did_open_file(file_name, 0, " LR 1,1");
    auto file = mngr.add_processor_file(file_name);

    // Prior to parsing, it should return default values;

    auto& fp = file->get_lsp_feature_provider();

    EXPECT_EQ(fp.definition(file_name, { 0, 5 }), location({ 0, 5 }, file_name));
    EXPECT_EQ(fp.references(file_name, { 0, 5 }), location_list());
    EXPECT_EQ(fp.hover(file_name, { 0, 5 }), "");
    EXPECT_EQ(fp.completion(file_name, { 0, 5 }, '\0', completion_trigger_kind::invoked), lsp::completion_list_s());

    EXPECT_EQ(file->get_hl_info(), semantics::lines_info());
    EXPECT_EQ(file->get_metrics(), performance_metrics());
}
