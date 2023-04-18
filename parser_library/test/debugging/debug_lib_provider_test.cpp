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

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../workspace/file_manager_mock.h"
#include "../workspace/library_mock.h"
#include "analyzer.h"
#include "debugging/debug_lib_provider.h"
#include "utils/resource_location.h"
#include "utils/task.h"

using namespace ::testing;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::debugging;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

namespace {

class debug_lib_provider_test : public Test
{
protected:
    std::shared_ptr<NiceMock<library_mock>> mock_lib = std::make_shared<NiceMock<library_mock>>();
    NiceMock<file_manager_mock> fm_mock;
    debug_lib_provider lib = debug_lib_provider({ mock_lib }, fm_mock);

    void analyze(analyzer& a) { a.co_analyze().run(); }
};

auto get_file_cortn(std::optional<std::string> result)
{
    return [result](
               const auto&) { return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(result); };
}

} // namespace

TEST_F(debug_lib_provider_test, parse_library)
{
    const std::string aaa_content = " MNOTE 'AAA'";
    const resource_location aaa_location("AAA");
    EXPECT_CALL(fm_mock, get_file_content(Eq(aaa_location))).WillOnce(Invoke(get_file_cortn(aaa_content)));
    EXPECT_CALL(*mock_lib, has_file(Eq("AAA"), _)).WillOnce(DoAll(SetArgPointee<1>(aaa_location), Return(true)));

    std::string input = " COPY AAA";
    analyzer a(input, analyzer_options(&lib));
    analyze(a);
    a.collect_diags();

    EXPECT_TRUE(matches_message_codes(a.diags(), { "MNOTE" }));
}

TEST_F(debug_lib_provider_test, has_library)
{
    EXPECT_CALL(*mock_lib, has_file(Eq("AAA"), _)).WillOnce(Return(true));
    EXPECT_CALL(*mock_lib, has_file(Eq("BBB"), _)).WillOnce(Return(false));

    EXPECT_TRUE(lib.has_library("AAA", nullptr));
    EXPECT_FALSE(lib.has_library("BBB", nullptr));
}

TEST_F(debug_lib_provider_test, get_library)
{
    const std::string aaa_content = "AAA content";
    const resource_location aaa_location("AAA");
    EXPECT_CALL(fm_mock, get_file_content(Eq(aaa_location))).WillOnce(Invoke(get_file_cortn(aaa_content)));

    EXPECT_CALL(*mock_lib, has_file(Eq("AAA"), _)).WillOnce(DoAll(SetArgPointee<1>(aaa_location), Return(true)));
    EXPECT_CALL(*mock_lib, has_file(Eq("BBB"), _)).WillOnce(Return(false));

    std::optional<std::pair<std::string, resource_location>> result;

    EXPECT_EQ(lib.get_library("AAA").run().value(), std::pair(aaa_content, aaa_location));

    EXPECT_EQ(lib.get_library("BBB").run().value(), std::nullopt);
}
