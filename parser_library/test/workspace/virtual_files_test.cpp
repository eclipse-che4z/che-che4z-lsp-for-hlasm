/*
 * Copyright (c) 2022 Broadcom.
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

#include <optional>
#include <utility>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../workspace_manager_response_mock.h"
#include "consume_diagnostics_mock.h"
#include "diagnosable_impl.h"
#include "file_manager_mock.h"
#include "preprocessor_options.h"
#include "utils/resource_location.h"
#include "virtual_file_monitor.h"
#include "workspace_manager.h"
#include "workspace_manager_response.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/file_manager_vfm.h"

using namespace ::testing;
using namespace hlasm_plugin::utils::path;

namespace {
class vf_mock : public virtual_file_monitor
{
public:
    MOCK_METHOD(virtual_file_handle, file_generated, (std::string_view content), (override));
};
} // namespace

TEST(virtual_files, callback_test_ainsert)
{
    vf_mock vf;

    EXPECT_CALL(vf, file_generated(std::string_view("A DS H\n"))).WillOnce(Return(virtual_file_handle()));

    std::string input = R"(
    AINSERT 'A DS H',BACK
)";
    analyzer a(input, analyzer_options(&vf));
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}

TEST(virtual_files, callback_test_preprocessor)
{
    vf_mock vf;

    EXPECT_CALL(vf, file_generated(Ne(std::string_view()))).WillOnce(Return(virtual_file_handle()));

    std::string input = R"(
)";
    analyzer a(input, analyzer_options(&vf, db2_preprocessor_options()));
    a.analyze();
    a.collect_diags();
    EXPECT_TRUE(a.diags().empty());
}

TEST(virtual_files, file_manager)
{
    constexpr std::string_view empty;
    constexpr std::string_view content = "content";
    const hlasm_plugin::utils::resource::resource_location related_workspace("workspace");
    file_manager_impl fm;

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    fm.put_virtual_file(0, content, related_workspace);

    EXPECT_EQ(fm.get_virtual_file(0), content);
    EXPECT_EQ(fm.get_virtual_file_workspace(0), related_workspace);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);
    EXPECT_EQ(fm.get_virtual_file_workspace(0).get_uri(), empty);
}
TEST(virtual_files, callback_test_ainsert_valid_vfm)
{
    vf_mock vf;

    EXPECT_CALL(vf, file_generated(std::string_view("A DC H\n")))
        .WillOnce(Return(virtual_file_handle(std::make_shared<const virtual_file_id>(0))));

    std::string input = R"(
    AINSERT 'A DC H',BACK
)";
    analyzer a(input, analyzer_options(&vf));
    a.analyze();
    a.collect_diags();

    const auto& d = a.diags();
    ASSERT_EQ(d.size(), 1);
    EXPECT_EQ(d[0].file_uri, "hlasm://0/AINSERT_1.hlasm");
}

TEST(virtual_files, file_manager_vfm)
{
    const hlasm_plugin::utils::resource::resource_location related_workspace("workspace");
    file_manager_mock fm;
    file_manager_vfm vfm(fm, related_workspace);

    constexpr std::string_view test_content = "test content";

    constexpr auto id_initial = ~0ULL;
    auto id = id_initial;

    EXPECT_CALL(fm, put_virtual_file(_, Eq(test_content), Eq(related_workspace))).WillOnce(SaveArg<0>(&id));
    EXPECT_CALL(fm, remove_virtual_file(Eq(std::cref(id)))).Times(1);

    auto file_id = vfm.file_generated(test_content).file_id();

    EXPECT_NE(id, id_initial);

    ASSERT_TRUE(file_id);
    EXPECT_EQ(file_id.value(), id);
}

TEST(virtual_files, workspace)
{
    auto ws_mngr = create_workspace_manager();
    ws_mngr->add_workspace("ws", "ws");
    std::string_view input = R"(
    AINSERT 'A DC H',BACK
)";
    ws_mngr->did_open_file("ws/file", 1, input.data(), input.size());
    ws_mngr->idle_handler();
    ws_mngr->did_close_file("ws/file");
    ws_mngr->idle_handler();
}

TEST(virtual_files, workspace_auto_cleanup)
{
    auto ws_mngr = create_workspace_manager();
    ws_mngr->add_workspace("ws", "ws");
    std::string_view input = R"(
    AINSERT 'A DC H',BACK
)";
    ws_mngr->did_open_file("ws/file", 1, input.data(), input.size());
    ws_mngr->idle_handler();
}

TEST(virtual_files, hover)
{
    diag_consumer_mock diag_mock;
    auto ws_mngr = create_workspace_manager();
    ws_mngr->add_workspace("ws", "ws");
    std::string_view input = R"(
MY  DSECT
    DS  F
    AINSERT 'A DC H',BACK
)";
    ws_mngr->register_diagnostics_consumer(&diag_mock);
    ws_mngr->did_open_file("ws/file", 1, input.data(), input.size());
    ws_mngr->idle_handler();

    ASSERT_EQ(diag_mock.diags.diagnostics_size(), 1);

    auto diag = diag_mock.diags.diagnostics(0);
    std::string vf = diag.file_uri();

    ASSERT_TRUE(vf.starts_with("hlasm://"));

    auto [resp, mock] =
        make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<sequence<char>>>);

    EXPECT_CALL(*mock, provide(Truly([](sequence<char> hover_text) {
        return std::string_view(hover_text).find("MY + X'4' (4)") != std::string::npos;
    })));

    ws_mngr->hover(vf.c_str(), position(0, 0), resp);
    ws_mngr->idle_handler();
}
