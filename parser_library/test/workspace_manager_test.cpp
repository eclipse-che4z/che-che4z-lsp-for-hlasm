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

#include "common_testing.h"
#include "debugging/debugger_configuration.h"
#include "lib_config.h"
#include "message_consumer_mock.h"
#include "nlohmann/json.hpp"
#include "utils/platform.h"
#include "workspace/consume_diagnostics_mock.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"
#include "workspace_manager_response_mock.h"

using namespace hlasm_plugin::parser_library;

TEST(workspace_manager, add_not_existing_workspace)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "not_existing");

    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, add_existing_workspace)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");

    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, did_open_file)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");

    std::string input_text = "label lr 1,2";
    ws_mngr->did_open_file("test/library/test_wks/some_file", 1, input_text.c_str(), input_text.size());
    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, did_change_file)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");
    std::string input = "label lr 1,2 remark";
    ws_mngr->did_open_file("test/library/test_wks/new_file", 1, input.c_str(), input.size());
    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);

    std::vector<document_change> changes;
    std::string new_text = "anop";
    changes.push_back(document_change({ { 0, 6 }, { 0, input.size() } }, new_text.c_str(), new_text.size()));

    ws_mngr->did_change_file("test/library/test_wks/new_file", 2, changes.data(), 1);
    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)1);

    std::vector<document_change> changes1;
    std::string new_text1 = "";
    changes1.push_back(document_change({ { 0, 6 }, { 0, 10 } }, new_text1.c_str(), new_text1.size()));

    ws_mngr->did_change_file("test/library/test_wks/new_file", 3, changes1.data(), 1);
    ws_mngr->idle_handler();

    EXPECT_GT(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, cancel_debugger_configuration_request)
{
    auto [p, impl] = make_workspace_manager_response(
        std::in_place_type<workspace_manager_response_mock<debugging::debugger_configuration>>);

    EXPECT_CALL(*impl, error(-104, ::testing::StrEq("Workspace removed")));

    auto ws_mngr = create_workspace_manager();
    auto& dc = ws_mngr->get_debugger_configuration_provider();

    ws_mngr->add_workspace("workspace", "not_existing");

    dc.provide_debugger_configuration(sequence<char>(std::string_view("not_existing/file")), p);

    ws_mngr->remove_workspace("not_existing");

    ws_mngr->idle_handler();
}
