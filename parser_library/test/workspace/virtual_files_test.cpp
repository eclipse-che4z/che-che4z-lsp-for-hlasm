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
    MOCK_METHOD(
        (std::pair<virtual_file_handle, std::string_view>), file_generated, (std::string_view content), (override));
};
} // namespace

TEST(virtual_files, callback_test_ainsert)
{
    vf_mock vf;

    EXPECT_CALL(vf, file_generated(std::string_view("A DS H\n")))
        .WillOnce(Return(std::pair<virtual_file_handle, std::string_view>({}, std::string_view("A DS H\n"))));

    std::string input = R"(
    AINSERT 'A DS H',BACK
)";
    analyzer a(input, analyzer_options(&vf));
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(virtual_files, callback_test_preprocessor)
{
    vf_mock vf;
    std::string saved_text;

    EXPECT_CALL(vf, file_generated(Ne(std::string_view())))
        .WillOnce(
            DoAll(SaveArg<0>(&saved_text), Return(std::pair<virtual_file_handle, std::string_view>({}, saved_text))));

    std::string input = R"(
)";
    analyzer a(input, analyzer_options(&vf, db2_preprocessor_options()));
    a.analyze();
    EXPECT_TRUE(a.diags().empty());
}

TEST(virtual_files, file_manager)
{
    constexpr std::string_view empty;
    constexpr std::string_view content = "content";
    file_manager_impl fm;

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    auto stored_text = fm.put_virtual_file(0, content);

    EXPECT_EQ(stored_text, content);
    EXPECT_EQ(fm.get_virtual_file(0), content);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);
}
TEST(virtual_files, callback_test_ainsert_valid_vfm)
{
    vf_mock vf;

    EXPECT_CALL(vf, file_generated(std::string_view("A DC H\n")))
        .WillOnce(Return(std::pair<virtual_file_handle, std::string_view>(
            virtual_file_handle(std::make_shared<const virtual_file_id>(0)), std::string_view("A DC H\n"))));

    std::string input = R"(
    AINSERT 'A DC H',BACK
)";
    analyzer a(input, analyzer_options(&vf));
    a.analyze();

    EXPECT_TRUE(matches_message_properties(a.diags(), { "hlasm://0/AINSERT_1.hlasm" }, &diagnostic::file_uri));
}

TEST(virtual_files, file_manager_vfm)
{
    file_manager_mock fm;
    file_manager_vfm vfm(fm);

    constexpr std::string_view test_content = "test content";

    constexpr auto id_initial = ~0ULL;
    auto id = id_initial;

    EXPECT_CALL(fm, put_virtual_file(_, Eq(test_content))).WillOnce(DoAll(SaveArg<0>(&id), Return(test_content)));
    EXPECT_CALL(fm, remove_virtual_file(Eq(std::cref(id)))).Times(1);

    auto [file_handle, file_text] = vfm.file_generated(test_content);

    EXPECT_EQ(file_text, test_content);

    auto file_id = file_handle.file_id();

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
    ws_mngr->did_open_file("ws/file", 1, input);
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
    ws_mngr->did_open_file("ws/file", 1, input);
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
    ws_mngr->did_open_file("ws/file", 1, input);
    ws_mngr->idle_handler();

    ASSERT_EQ(diag_mock.diags.size(), 1);

    const auto& diag = diag_mock.diags[0];
    const auto& vf = diag.file_uri;

    ASSERT_TRUE(vf.starts_with("hlasm://"));

    auto [resp, mock] =
        make_workspace_manager_response(std::in_place_type<workspace_manager_response_mock<std::string_view>>);

    EXPECT_CALL(*mock, provide(Truly([](std::string_view hover_text) {
        return hover_text.find("MY + X'4' (4)") != std::string::npos;
    })));

    ws_mngr->hover(vf, position(0, 0), resp);
    ws_mngr->idle_handler();
}
