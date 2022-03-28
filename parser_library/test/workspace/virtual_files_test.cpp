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
#include "preprocessor_options.h"
#include "virtual_file_monitor.h"
#include "workspaces/file_manager_impl.h"

using namespace ::testing;

namespace {
class vf_moc : public virtual_file_monitor
{
public:
    MOCK_METHOD(virtual_file_handle, file_generated, (std::string_view content), (override));
};
} // namespace

TEST(virtual_files, callback_test_ainsert)
{
    vf_moc vf;

    virtual_file_id expected_id;
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
    vf_moc vf;

    virtual_file_id expected_id;
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
    file_manager_impl fm;

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);

    fm.put_virtual_file(0, content);

    EXPECT_EQ(fm.get_virtual_file(0), content);

    fm.remove_virtual_file(0);

    EXPECT_EQ(fm.get_virtual_file(0), empty);
}
TEST(virtual_files, callback_test_ainsert_valid_vfm)
{
    vf_moc vf;

    virtual_file_id expected_id;
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
    EXPECT_EQ(d[0].file_name, "hlasm://0/AINSERT:1");
}
