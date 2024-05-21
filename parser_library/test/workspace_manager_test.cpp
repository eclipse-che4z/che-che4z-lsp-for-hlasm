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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common_testing.h"
#include "debugging/debugger_configuration.h"
#include "lib_config.h"
#include "message_consumer_mock.h"
#include "nlohmann/json.hpp"
#include "utils/platform.h"
#include "workspace/consume_diagnostics_mock.h"
#include "workspace_manager.h"
#include "workspace_manager_external_file_requests.h"
#include "workspace_manager_response.h"
#include "workspace_manager_response_mock.h"

using namespace hlasm_plugin::parser_library;
using namespace ::testing;

TEST(workspace_manager, add_not_existing_workspace)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "not_existing");

    ws_mngr->idle_handler();

    EXPECT_TRUE(consumer.diags.empty());
}

TEST(workspace_manager, add_existing_workspace)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");

    ws_mngr->idle_handler();

    EXPECT_TRUE(consumer.diags.empty());
}

TEST(workspace_manager, did_open_file)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");

    std::string input_text = "label lr 1,2";
    ws_mngr->did_open_file("test/library/test_wks/some_file", 1, input_text);
    ws_mngr->idle_handler();

    EXPECT_TRUE(consumer.diags.empty());
}

TEST(workspace_manager, did_change_file)
{
    auto ws_mngr = create_workspace_manager();
    diag_consumer_mock consumer;
    ws_mngr->register_diagnostics_consumer(&consumer);

    ws_mngr->add_workspace("workspace", "test/library/test_wks");
    std::string input = "label lr 1,2 remark";
    ws_mngr->did_open_file("test/library/test_wks/new_file", 1, input);
    ws_mngr->idle_handler();

    EXPECT_TRUE(consumer.diags.empty());

    std::vector<document_change> changes;
    std::string new_text = "anop";
    changes.push_back(document_change({ { 0, 6 }, { 0, input.size() } }, new_text));

    ws_mngr->did_change_file("test/library/test_wks/new_file", 2, changes);
    ws_mngr->idle_handler();

    EXPECT_EQ(consumer.diags.size(), (size_t)1);

    std::vector<document_change> changes1;
    std::string new_text1 = "";
    changes1.push_back(document_change({ { 0, 6 }, { 0, 10 } }, new_text1));

    ws_mngr->did_change_file("test/library/test_wks/new_file", 3, changes1);
    ws_mngr->idle_handler();

    EXPECT_FALSE(consumer.diags.empty());
}

TEST(workspace_manager, cancel_debugger_configuration_request)
{
    auto [p, impl] = make_workspace_manager_response(
        std::in_place_type<workspace_manager_response_mock<debugging::debugger_configuration>>);

    EXPECT_CALL(*impl, error(-104, ::testing::StrEq("Workspace removed")));

    auto ws_mngr = create_workspace_manager();
    auto& dc = ws_mngr->get_debugger_configuration_provider();

    ws_mngr->add_workspace("workspace", "not_existing");

    dc.provide_debugger_configuration("not_existing/file", p);

    ws_mngr->remove_workspace("not_existing");

    ws_mngr->idle_handler();
}

struct workspace_manager_external_file_requests_mock : public workspace_manager_external_file_requests
{
    MOCK_METHOD(void, read_external_file, (std::string_view url, workspace_manager_response<std::string_view> content));
    MOCK_METHOD(void,
        read_external_directory,
        (std::string_view url,
            workspace_manager_response<workspace_manager_external_directory_result> members,
            bool subdir));
};

TEST(workspace_manager, extended_scheme_allowed_list)
{
    const std::string_view uri = "test:/dir/file";
    NiceMock<workspace_manager_external_file_requests_mock> ext_mock;
    diag_consumer_mock diags;

    auto ws_mngr = create_workspace_manager(&ext_mock, true);
    ws_mngr->register_diagnostics_consumer(&diags);

    bool called = false;
    EXPECT_CALL(ext_mock, read_external_file).WillRepeatedly(Invoke([&called](auto, auto r) {
        called = true;
        r.error(-1, "");
    }));

    ws_mngr->did_open_file(uri, 1, " ERROR");

    ws_mngr->idle_handler();

    EXPECT_TRUE(matches_message_codes(diags.diags, { "SUP" })); // quiet due to allow list
    EXPECT_FALSE(called);

    ws_mngr->did_close_file(uri);
    ws_mngr->idle_handler();

    ws_mngr->add_workspace("dir", "test:/dir");
    ws_mngr->did_open_file(uri, 1, " ERROR");

    ws_mngr->idle_handler();

    EXPECT_FALSE(contains_message_codes(diags.diags, { "SUP" })); // no longer quiet
    EXPECT_FALSE(diags.diags.empty()); // not suppressed
    EXPECT_TRUE(called);
}

TEST(workspace_manager, implicit_configuration)
{
    const std::string_view uri = "untitled:file1";
    NiceMock<workspace_manager_external_file_requests_mock> ext_mock;
    diag_consumer_mock diags;

    auto ws_mngr = create_workspace_manager(&ext_mock, true);
    ws_mngr->register_diagnostics_consumer(&diags);
    ws_mngr->add_workspace("dir", "test:/dir");
    ws_mngr->configuration_changed({},
        R"({"hlasm":{"proc_grps":{"pgroups":[{"name":"P1","libs":["test:/dir/macs/"]}]},"pgm_conf":{"pgms":[{"program":"**","pgroup":"P1"}]}}})");

    EXPECT_CALL(ext_mock, read_external_file).WillRepeatedly(Invoke([](auto, auto r) { r.error(-1, ""); }));
    EXPECT_CALL(ext_mock, read_external_directory(StrEq("test:/dir/macs/"), _, _))
        .WillOnce(Invoke([](auto, auto r, auto) {
            static constexpr std::string_view resp[] = { "test:/dir/macs/MAC" };
            r.provide(workspace_manager_external_directory_result { .member_urls = resp });
        }));

    ws_mngr->did_open_file("test:/dir/macs/MAC", 1, R"( MACRO
    MAC
    MNOTE 'Hello'
    MEND
)");
    ws_mngr->did_open_file(uri, 1, " MAC");

    ws_mngr->idle_handler();

    EXPECT_TRUE(matches_message_text(diags.diags, { "Hello" }));
}
