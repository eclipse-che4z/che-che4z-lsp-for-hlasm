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
#include "diagnosable_impl.h"
#include "preprocessor_options.h"
#include "utils/resource_location.h"
#include "virtual_file_monitor.h"
#include "workspace_manager.h"
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

class fm_mock : public file_manager, public diagnosable_impl
{
public:
    void collect_diags() const override
    {
        // nothing to do
    }
    MOCK_METHOD(file_ptr, add_file, (const file_location&), (override));
    MOCK_METHOD(processor_file_ptr, add_processor_file, (const file_location&), (override));
    MOCK_METHOD(processor_file_ptr, get_processor_file, (const file_location&), (override));
    MOCK_METHOD(void, remove_file, (const file_location&), (override));
    MOCK_METHOD(file_ptr, find, (const file_location& key), (const override));
    MOCK_METHOD(processor_file_ptr, find_processor_file, (const file_location& key), (override));
    MOCK_METHOD(list_directory_result,
        list_directory_files,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const override));
    MOCK_METHOD(list_directory_result,
        list_directory_subdirs_and_symlinks,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const override));
    MOCK_METHOD(std::string,
        canonical,
        (const hlasm_plugin::utils::resource::resource_location& res_loc, std::error_code& ec),
        (const override));
    MOCK_METHOD(
        bool, file_exists, (const hlasm_plugin::utils::resource::resource_location& file_loc), (const override));
    MOCK_METHOD(bool,
        lib_file_exists,
        (const hlasm_plugin::utils::resource::resource_location& lib_root, std::string_view file_name),
        (const override));
    MOCK_METHOD(bool, dir_exists, (const hlasm_plugin::utils::resource::resource_location& dir_loc), (const override));
    MOCK_METHOD(
        void, did_open_file, (const file_location& document_loc, version_t version, std::string text), (override));
    MOCK_METHOD(void,
        did_change_file,
        (const file_location& document_loc, version_t version, const document_change* changes, size_t ch_size),
        (override));
    MOCK_METHOD(void, did_close_file, (const file_location& document_loc), (override));
    MOCK_METHOD(void,
        put_virtual_file,
        (unsigned long long id,
            std::string_view text,
            hlasm_plugin::utils::resource::resource_location related_workspace),
        (override));
    MOCK_METHOD(void, remove_virtual_file, (unsigned long long id), (override));
    MOCK_METHOD(std::string, get_virtual_file, (unsigned long long id), (const override));
    MOCK_METHOD(hlasm_plugin::utils::resource::resource_location,
        get_virtual_file_workspace,
        (unsigned long long id),
        (const override));
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
    fm_mock fm;
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
    workspace_manager wm;
    wm.add_workspace("ws", "ws");
    std::string_view input = R"(
    AINSERT 'A DC H',BACK
)";
    wm.did_open_file("ws/file", 1, input.data(), input.size());
    wm.did_close_file("ws/file");
}

TEST(virtual_files, workspace_auto_cleanup)
{
    workspace_manager wm;
    wm.add_workspace("ws", "ws");
    std::string_view input = R"(
    AINSERT 'A DC H',BACK
)";
    wm.did_open_file("ws/file", 1, input.data(), input.size());
}

TEST(virtual_files, hover)
{
    class diag_consumer_mock : public diagnostics_consumer
    {
    public:
        // Inherited via diagnostics_consumer
        void consume_diagnostics(diagnostic_list diagnostics) override { diags = diagnostics; }

        diagnostic_list diags;
    } diag_mock;

    workspace_manager wm;
    wm.add_workspace("ws", "ws");
    std::string_view input = R"(
MY  DSECT
    DS  F
    AINSERT 'A DC H',BACK
)";
    wm.register_diagnostics_consumer(&diag_mock);
    wm.did_open_file("ws/file", 1, input.data(), input.size());

    ASSERT_EQ(diag_mock.diags.diagnostics_size(), 1);

    auto diag = diag_mock.diags.diagnostics(0);
    std::string vf = diag.file_uri();

    ASSERT_TRUE(vf.starts_with("hlasm://"));

    std::string hover_text(wm.hover(vf.c_str(), position(0, 0)));

    EXPECT_NE(hover_text.find("MY + X'4' (4)"), std::string::npos);
}
