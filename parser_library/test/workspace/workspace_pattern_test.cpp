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

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using hlasm_plugin::utils::platform::is_windows;

namespace {
const std::string pgroups_file_pattern_absolute = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "C:\\Temp\\Lib", "C:\\Temp\\Lib2\\Libs\\**" ]
    }
  ]
})"
                                                               : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "/home/Temp/Lib", "/home/Temp/Lib2/Libs/**" ]
    }
  ]
})";

const std::string pgroups_file_pattern_relative = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "pattern_test\\libs\\**\\" ]
    }
  ]
})"
                                                               : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "pattern_test/libs/**/" ]
    }
  ]
})";

const std::string pgroups_file_pattern_relative_2 = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "pattern_test\\libs\\**" ]
    }
  ]
})"
                                                                 : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "pattern_test/libs/**" ]
    }
  ]
})";

const std::string pgroups_file_pattern_relative_3 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "*" ]
    }
  ]
})";

const std::string pgroups_file_pattern_uri = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///C%3A/User/ws/pattern_test/libs/**" ]
    }
  ]
})"
                                                          : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///home/User/ws/pattern_test/libs/**" ]
    }
  ]
})";

const std::string pgroups_file_pattern_uri_2 = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///C%3A/User/**/pattern_*est/libs/sublib1", "file:///C%3A/User/ws/pattern_test/libs/sublib2" ]
    }
  ]
})"
                                                            : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///home/User/**/pattern_*est/libs/sublib1", "file:///home/User/ws/pattern_test/libs/sublib2" ]
    }
  ]
})";

const std::string pgroups_file_pattern_uri_non_standard_0 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:C%3A/User/ws/pattern_test/libs/**" ]
    }
  ]
})";

const std::string pgroups_file_pattern_uri_non_standard_1 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:/C%3A/User/ws/pattern_test/libs/**" ]
    }
  ]
})";

const std::string pgroups_file_pattern_uri_non_standard_2 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file://C%3A/User/ws/pattern_test/libs/**" ]
    }
  ]
})";

const std::string pgroups_file_combination = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "C:\\Temp\\Lib", "C:\\Temp\\Lib2\\Libs\\**", "different_libs", "different_libs2\\Libs\\**", "file:///C%3A/User/**/pattern_*est/libs/sublib1", "file:///C%3A/User/ws/pattern_test/libs/sublib2" ]
    }
  ]
})"
                                                          : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "/home/Temp/Lib", "/home/Temp/Lib2/Libs/**", "different_libs", "different_libs2/Libs/**", "file:///home/User/**/pattern_*est/libs/sublib1", "file:///home/User/ws/pattern_test/libs/sublib2" ]
    }
  ]
})";

const std::string pgmconf_file_platform_dependent_slashes = is_windows() ? R"({
  "pgms": [
	{
      "program": "pattern_test\\source",
      "pgroup": "P1"
    }
  ]
})"
                                                                         : R"({
  "pgms": [
	{
      "program": "pattern_test/source",
      "pgroup": "P1"
    }
  ]
})";

const std::string pgmconf_file_platform_independent_slashes = R"({
  "pgms": [
	{
      "program": "pattern_test/source",
      "pgroup": "P1"
    }
  ]
})";

const std::string source_txt = R"(         MACRO
         MAC
         MAC1
         MAC2
         MEND

         MAC
         MAC1
         MAC2

         END)";

enum class pgroup_variants
{
    ABSOLUTE,
    RELATIVE,
    RELATIVE_2,
    RELATIVE_3,
    URI,
    URI_2,
    URI_NON_STANDARD_0,
    URI_NON_STANDARD_1,
    URI_NON_STANDARD_2,
    COMBINATION
};

enum class pgmconf_variants
{
    PLATFORM_DEPENDENT,
    PLATFORM_INDEPENDENT
};

const auto root_dir_loc = is_windows() ? resource_location("file:///C%3A/") : resource_location("file:///home/");
const auto user_dir_loc = resource_location::join(root_dir_loc, "User/");

const auto ws_loc = resource_location::join(user_dir_loc, "ws/");
const auto hlasmplugin_folder_loc(resource_location::join(ws_loc, ".hlasmplugin/"));
const auto proc_grps_loc(resource_location::join(hlasmplugin_folder_loc, "proc_grps.json"));
const auto pgm_conf_loc(resource_location::join(hlasmplugin_folder_loc, "pgm_conf.json"));

const auto pattern_test_dir_loc = resource_location::join(ws_loc, "pattern_test/");
const auto pattern_est_dir_loc = resource_location::join(ws_loc, "pattern_est/");
const auto patter_test_dir_loc = resource_location::join(ws_loc, "patter_test/");

const auto pattern_test_source_loc(resource_location::join(pattern_test_dir_loc, "source"));
const auto pattern_test_lib_loc(resource_location::join(pattern_test_dir_loc, "libs/"));
const auto pattern_test_lib_sublib1_loc(resource_location::join(pattern_test_lib_loc, "sublib1/"));
const auto pattern_test_lib_sublib2_loc(resource_location::join(pattern_test_lib_loc, "sublib2/"));
const auto pattern_test_macro1_loc(resource_location::join(pattern_test_lib_sublib1_loc, "mac1"));
const auto pattern_test_macro2_loc(resource_location::join(pattern_test_lib_sublib2_loc, "mac2"));

const auto temp_lib_loc = resource_location::join(root_dir_loc, "Temp/Lib/");
const auto temp_lib2_libs_loc = resource_location::join(root_dir_loc, "Temp/Lib2/Libs/");

const auto different_libs_loc = resource_location::join(ws_loc, "different_libs/");
const auto different_libs2_libs_loc = resource_location::join(ws_loc, "different_libs2/Libs/");

const auto different_libs2_libs_subdir = resource_location::join(different_libs2_libs_loc, "subdir/");
const auto temp_lib2_libs_subdir = resource_location::join(temp_lib2_libs_loc, "subdir/");

const char* pattern_lib_sublib1_abs_path =
    is_windows() ? "C:\\\\User\\ws\\pattern_test\\libs\\sublib1\\" : "/home/User/ws/pattern_test/libs/sublib1/";
const char* pattern_lib_sublib2_abs_path =
    is_windows() ? "c:\\\\User\\ws\\pAttErn_test\\libs\\sublib2\\" : "/home/User/ws/pattern_test/libs/sublib2/";

class file_manager_lib_pattern : public file_manager_impl
{
    std::string get_pgroup_file(pgroup_variants variant)
    {
        switch (variant)
        {
            case pgroup_variants::ABSOLUTE:
                return pgroups_file_pattern_absolute;
            case pgroup_variants::RELATIVE:
                return pgroups_file_pattern_relative;
            case pgroup_variants::RELATIVE_2:
                return pgroups_file_pattern_relative_2;
            case pgroup_variants::RELATIVE_3:
                return pgroups_file_pattern_relative_3;
            case pgroup_variants::URI:
                return pgroups_file_pattern_uri;
            case pgroup_variants::URI_2:
                return pgroups_file_pattern_uri_2;
            case pgroup_variants::URI_NON_STANDARD_0:
                return pgroups_file_pattern_uri_non_standard_0;
            case pgroup_variants::URI_NON_STANDARD_1:
                return pgroups_file_pattern_uri_non_standard_1;
            case pgroup_variants::URI_NON_STANDARD_2:
                return pgroups_file_pattern_uri_non_standard_2;
            case pgroup_variants::COMBINATION:
                return pgroups_file_combination;
        }
        throw std::logic_error("Not implemented");
    }

protected:
    std::string get_pgmconf_file(pgmconf_variants variant)
    {
        switch (variant)
        {
            case pgmconf_variants::PLATFORM_DEPENDENT:
                return pgmconf_file_platform_dependent_slashes;
            case pgmconf_variants::PLATFORM_INDEPENDENT:
                return pgmconf_file_platform_independent_slashes;
        }
        throw std::logic_error("Not implemented");
    }

    file_manager_lib_pattern() = default;

public:
    file_manager_lib_pattern(pgroup_variants pgroup_variant, pgmconf_variants pgmconf_variant)
    {
        did_open_file(proc_grps_loc, 1, get_pgroup_file(pgroup_variant));
        did_open_file(pgm_conf_loc, 1, get_pgmconf_file(pgmconf_variant));
        did_open_file(pattern_test_source_loc, 1, source_txt);
    }

    MOCK_METHOD(list_directory_result,
        list_directory_files,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const override));

    std::string canonical(
        const hlasm_plugin::utils::resource::resource_location& res_loc, std::error_code&) const override
    {
        return res_loc.get_path();
    }

    list_directory_result list_directory_subdirs_and_symlinks(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == user_dir_loc)
            return { { { "ws", ws_loc } }, hlasm_plugin::utils::path::list_directory_rc::done };

        if (location == ws_loc)
            return { { { "pattern_est", pattern_est_dir_loc },
                         { "pattern_test", pattern_test_dir_loc },
                         { "patter_test", patter_test_dir_loc } },
                hlasm_plugin::utils::path::list_directory_rc::done };

        if (location == pattern_test_dir_loc)
            return { { { "libs", pattern_test_lib_loc } }, hlasm_plugin::utils::path::list_directory_rc::done };

        if (location == pattern_test_lib_loc)
            return { { { pattern_lib_sublib1_abs_path, pattern_test_lib_sublib1_loc },
                         { pattern_lib_sublib2_abs_path, pattern_test_lib_sublib2_loc } },
                hlasm_plugin::utils::path::list_directory_rc::done };

        if (location == different_libs2_libs_loc)
            return { { { "subdir", different_libs2_libs_subdir } },
                hlasm_plugin::utils::path::list_directory_rc::done };

        if (location == temp_lib2_libs_loc)
            return { { { "subdir", temp_lib2_libs_subdir } }, hlasm_plugin::utils::path::list_directory_rc::done };

        return { {}, hlasm_plugin::utils::path::list_directory_rc::done };
    }

    bool dir_exists(const hlasm_plugin::utils::resource::resource_location&) const override { return true; }
};
} // namespace

void verify_absolute(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::ABSOLUTE, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(temp_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(temp_lib2_libs_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(temp_lib2_libs_subdir))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_relative_1(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::RELATIVE, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_relative_2(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::RELATIVE_2, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_relative_3(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::RELATIVE_3, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_dir_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(patter_test_dir_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_est_dir_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_uri_1(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::URI, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_uri_2(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::URI_2, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_uri_non_standard(pgroup_variants pgroup_variant, pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variant, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

void verify_combination(pgmconf_variants pgmconf_variant)
{
    file_manager_lib_pattern file_manager(pgroup_variants::COMBINATION, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();

    EXPECT_CALL(file_manager, list_directory_files(temp_lib_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(temp_lib2_libs_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(temp_lib2_libs_subdir))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(different_libs_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(different_libs2_libs_loc))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(different_libs2_libs_subdir))
        .WillOnce(::testing::Return(list_directory_result { {}, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));
    EXPECT_CALL(file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Return(list_directory_result {
            { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done }));

    ws.did_open_file(pattern_test_source_loc);
}

TEST(workspace_pattern_test, absolute) { verify_absolute(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, absolute_independent) { verify_absolute(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, relative_1) { verify_relative_1(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_1_independent) { verify_relative_1(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, relative_2) { verify_relative_2(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_2_independent) { verify_relative_2(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, relative_3) { verify_relative_3(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_3_independent) { verify_relative_3(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_1) { verify_uri_1(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, uri_1_independent) { verify_uri_1(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_2) { verify_uri_2(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, uri_2_independent) { verify_uri_2(pgmconf_variants::PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_non_standard_0)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_0, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_0_independent)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_0, pgmconf_variants::PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_1)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_1, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_1_independent)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_1, pgmconf_variants::PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_2)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_2, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_2_independent)
{
    if (hlasm_plugin::utils::platform::is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_2, pgmconf_variants::PLATFORM_INDEPENDENT);
}
TEST(workspace_pattern_test, combination) { verify_combination(pgmconf_variants::PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, combination_independent) { verify_combination(pgmconf_variants::PLATFORM_INDEPENDENT); }

namespace {

const std::string pgroups_file_inf = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///C%3A/User/ws/symlinks/inf0/**" ]
    }
  ]
})"
                                                  : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///home/User/ws/symlinks/inf0/**" ]
    }
  ]
})";

const std::string pgroups_file_inf_2 = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///C%3A/User/ws/symlinks/inf/**" ]
    }
  ]
})"
                                                    : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///home/User/ws/symlinks/inf/**" ]
    }
  ]
})";

const std::string pgroups_file_canonical = is_windows() ? R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///C%3A/User/ws/canonical/**" ]
    }
  ]
})"
                                                        : R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "file:///home/User/ws/canonical/**" ]
    }
  ]
})";

enum class pgroup_symlinks_variants
{
    INFINIT,
    INFINIT_2,
    CANONICAL
};

class file_manager_infinit_loop : public file_manager_lib_pattern
{
    std::string get_pgroup_file(pgroup_symlinks_variants variant)
    {
        switch (variant)
        {
            case pgroup_symlinks_variants::INFINIT:
                return pgroups_file_inf;
            case pgroup_symlinks_variants::INFINIT_2:
                return pgroups_file_inf_2;
            case pgroup_symlinks_variants::CANONICAL:
                return pgroups_file_canonical;
        }
        throw std::logic_error("Not implemented");
    }

public:
    file_manager_infinit_loop(pgroup_symlinks_variants pgroup_variant, pgmconf_variants pgmconf_variant)
    {
        did_open_file(proc_grps_loc, 1, get_pgroup_file(pgroup_variant));
        did_open_file(pgm_conf_loc, 1, get_pgmconf_file(pgmconf_variant));
        did_open_file(pattern_test_source_loc, 1, source_txt);
    }

    std::string canonical(const hlasm_plugin::utils::resource::resource_location&, std::error_code&) const override
    {
        return "canonical";
    }

    list_directory_result list_directory_subdirs_and_symlinks(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location.get_uri().ends_with("/inf") || location.get_uri().ends_with("/inf/"))
        {
            // Just append a dir and return
            auto new_loc = resource_location::join(location, "inf");
            return { { { new_loc.get_uri(), new_loc } }, hlasm_plugin::utils::path::list_directory_rc::done };
        }

        if (location.get_uri().ends_with("/inf0/") || location.get_uri().ends_with("/inf1")
            || location.get_uri().ends_with("/inf2") || location.get_uri().ends_with("/inf3")
            || location.get_uri().ends_with("/inf4") || location.get_uri().ends_with("/inf5"))
        {
            // Just append a dir and return
            auto inf1 = resource_location::join(location, "inf1");
            auto inf2 = resource_location::join(location, "inf2");
            auto inf3 = resource_location::join(location, "inf3");
            auto inf4 = resource_location::join(location, "inf4");
            auto inf5 = resource_location::join(location, "inf5");
            return { { { inf1.get_uri(), inf1 },
                         { inf2.get_uri(), inf2 },
                         { inf3.get_uri(), inf3 },
                         { inf4.get_uri(), inf4 },
                         { inf5.get_uri(), inf5 } },
                hlasm_plugin::utils::path::list_directory_rc::done };
        }

        if (location.get_uri().ends_with("/canonical/") || location.get_uri().ends_with("/canonical"))
        {
            auto can = resource_location::join(location, "can1");
            return { { { "can1", can } }, hlasm_plugin::utils::path::list_directory_rc::done };
        }

        if (location.get_uri().ends_with("/can1"))
        {
            auto can = resource_location::join(location, "can2");
            return { { { "can2", can } }, hlasm_plugin::utils::path::list_directory_rc::done };
        }

        if (location.get_uri().ends_with("/can2"))
        {
            auto can = resource_location::join(location, "can3");
            return { { { "can3", can } }, hlasm_plugin::utils::path::list_directory_rc::done };
        }

        if (location.get_uri().ends_with("/can3"))
        {
            auto can = resource_location::join(location, "canonical");
            return { { { "canonical", can } }, hlasm_plugin::utils::path::list_directory_rc::done };
        }


        return { {}, hlasm_plugin::utils::path::list_directory_rc::done };
    }
};
} // namespace

void verify_infinit_loop(pgroup_symlinks_variants pgroup_variant, pgmconf_variants pgmconf_variant)
{
    ::testing::NiceMock<file_manager_infinit_loop> file_manager(pgroup_variant, pgmconf_variant);
    lib_config config;

    workspace ws(ws_loc, "workspace_name", file_manager, config);
    ws.open();
    ws.did_open_file(pattern_test_source_loc);

    // No explicit expectations - it is just expected that this will not crash or end up in a loop.
}

TEST(workspace_pattern_test, infinit_loop_general)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT, pgmconf_variants::PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_2)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT_2, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_2_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT_2, pgmconf_variants::PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_canonical)
{
    verify_infinit_loop(pgroup_symlinks_variants::CANONICAL, pgmconf_variants::PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_canonical_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::CANONICAL, pgmconf_variants::PLATFORM_INDEPENDENT);
}