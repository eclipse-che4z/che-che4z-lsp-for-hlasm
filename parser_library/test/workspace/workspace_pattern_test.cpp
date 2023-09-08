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
#include "empty_configs.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using hlasm_plugin::utils::platform::is_windows;

namespace {
const std::string file_scheme_user_dir = is_windows() ? "file:///C:/User/" : "file:///home/User/";
const std::string file_scheme_user_dir_encoded = is_windows() ? "file:///C%3A/User/" : "file:///home/User/";

std::string pgroups_generator(std::vector<std::string> lib_paths)
{
    std::string lib_path;
    bool first = true;

    for (auto path : lib_paths)
    {
        if (!first)
            lib_path.append(", ");

        first = false;

        lib_path.push_back('"');
        lib_path.append(path);
        lib_path.push_back('"');
    }

    return R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ )"
        + lib_path + R"( ]
    }
  ]
})";
}

std::string pgmconf_generator(std::string program)
{
    return R"({
  "pgms": [
	{
      "program": ")"
        + std::string(program) + R"(",
      "pgroup": "P1"
    }
  ]
})";
}

const std::string pgroups_file_pattern_absolute = is_windows()
    ? pgroups_generator({ "C:\\\\Temp\\\\Lib", "C:\\\\Temp\\\\Lib2\\\\L%25bs;\\\\**" })
    : pgroups_generator({ "/home/Temp/Lib", "/home/Temp/Lib2/L%25bs;/**" });

const std::string pgroups_file_pattern_relative = is_windows()
    ? pgroups_generator({ "product\\\\pattern_test\\\\l%25bs;\\\\**\\\\" })
    : pgroups_generator({ "product/pattern_test/l%25bs;/**/" });

const std::string pgroups_file_pattern_relative_2 = is_windows()
    ? pgroups_generator({ "product\\\\pattern_test\\\\l%25bs;\\\\**" })
    : pgroups_generator({ "product/pattern_test/l%25bs;/**" });

const std::string pgroups_file_pattern_relative_3 = pgroups_generator({ "product/*" });

const std::string pgroups_file_pattern_uri =
    pgroups_generator({ file_scheme_user_dir_encoded + "ws/product/pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_uri_2 =
    pgroups_generator({ file_scheme_user_dir_encoded + "**/pattern_*est/l%2525bs;/sublib1",
        file_scheme_user_dir_encoded + "ws/product/pattern_test/l%2525bs;/sublib2" });

const std::string pgroups_file_pattern_uri_3 =
    pgroups_generator({ file_scheme_user_dir_encoded + "ws/product/pattern_?est/**/sublib1",
        file_scheme_user_dir_encoded + "w?/product/pattern_test/l%2525bs;/sublib2" });

const std::string pgroups_file_pattern_uri_non_standard_0 =
    pgroups_generator({ "file:C%3A/User/ws/product/pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_uri_non_standard_1 =
    pgroups_generator({ "file:/C%3A/User/ws/product/pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_uri_non_standard_2 =
    pgroups_generator({ "file://C%3A/User/ws/product/pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_combination = is_windows()
    ? pgroups_generator({ "C:\\\\Temp\\\\Lib",
        "C:\\\\Temp\\\\Lib2\\\\L%25bs;\\\\**",
        "different_libs",
        "different_libs2\\\\Libs\\\\**",
        file_scheme_user_dir_encoded + "**/pattern_?est/l%2525bs;/sublib1",
        file_scheme_user_dir_encoded + "ws/product/pattern_test/l%2525bs;/sublib2" })
    : pgroups_generator({ "/home/Temp/Lib",
        "/home/Temp/Lib2/L%25bs;/**",
        "different_libs",
        "different_libs2/Libs/**",
        file_scheme_user_dir_encoded + "**/pattern_?est/l%2525bs;/sublib1",
        file_scheme_user_dir_encoded + "ws/product/pattern_test/l%2525bs;/sublib2" });

const std::string pgroups_file_pattern_double_dot_absolute =
    pgroups_generator({ file_scheme_user_dir_encoded + "ws/product/../product/pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_double_dot_relative_1 =
    pgroups_generator({ "../ws/product/pattern_test/l%25bs;/**" });

const std::string pgroups_file_pattern_double_dot_relative_2 =
    pgroups_generator({ "../../User/ws/product/pattern_test/l%25bs;/**" });

const std::string pgroups_file_pattern_double_dot_relative_3 =
    pgroups_generator({ "../ws/../ws/product/pattern_test/l%25bs;/**" });

const std::string pgroups_file_pattern_double_dot_absolute_wildcard =
    pgroups_generator({ file_scheme_user_dir_encoded + "ws/**/../pattern_test/l%2525bs;/**" });

const std::string pgroups_file_pattern_double_dot_relative_wildcard =
    pgroups_generator({ "**/../pattern_test/l%25bs;/**" });

const std::string pgmconf_file_relative_platform_dependent_slashes = is_windows()
    ? pgmconf_generator("product\\\\pattern_test\\\\s%25urce;")
    : pgmconf_generator("product/pattern_test/s%25urce;");

const std::string pgmconf_file_relative_platform_independent_slashes =
    pgmconf_generator("product/pattern_test/s%25urce;");

const std::string pgmconf_file_relative_wild =
    is_windows() ? pgmconf_generator("product\\\\patter*s*\\\\*") : pgmconf_generator("product/patter*s*/*");

const std::string pgmconf_file_absolute = is_windows()
    ? pgmconf_generator("C:/User/ws/product/pattern_test/s%25urce;")
    : pgmconf_generator("/home/User/ws/product/pattern_test/s%25urce;");

const std::string pgmconf_file_absolute_wild = is_windows()
    ? pgmconf_generator("C:/User/ws/product/pattern_test/s%25urce?")
    : pgmconf_generator("/home/User/ws/product/pattern_test/s%25urce?");

const std::string pgmconf_file_uri_full =
    pgmconf_generator(file_scheme_user_dir_encoded + "ws/product/pattern_test/s%2525urce%3B");

const std::string pgmconf_file_platform_uri_wild_01 =
    pgmconf_generator(file_scheme_user_dir + "ws/product/*/s%2525urce%3B");

const std::string pgmconf_file_platform_uri_wild_02 =
    pgmconf_generator(file_scheme_user_dir + "ws/product/pattern_*est/s%2525urce%3B");

const std::string pgmconf_file_platform_uri_wild_03 =
    pgmconf_generator(file_scheme_user_dir + "ws/product/pattern_?est/s%2525urce%3B");

const std::string pgmconf_file_platform_uri_wild_04 =
    pgmconf_generator(file_scheme_user_dir + "ws/product/pattern_?est/s*rce%3B");

const std::string pgmconf_file_platform_uri_wild_05 =
    pgmconf_generator(file_scheme_user_dir + "**/pattern_?est/s?25urce?");

const std::string pgmconf_file_platform_file_scheme_non_standard_0 =
    pgmconf_generator("file:c%3A/User/ws/product/pattern_test/s%2525u*ce%3B");

const std::string pgmconf_file_platform_file_scheme_non_standard_1 =
    pgmconf_generator("file:/C%3a/User/ws/product/pattern_**/s%2525urce%3B");

const std::string pgmconf_file_platform_file_scheme_non_standard_2 =
    pgmconf_generator("file://c:/User/**/patter*_test/s%2525urce%3B");

const std::string pgmconf_file_double_dot_absolute =
    pgmconf_generator({ file_scheme_user_dir_encoded + "ws/../ws/product/pattern_test/**" });

const std::string pgmconf_file_double_dot_relative_1 = pgmconf_generator({ "../ws/product/pattern_test/**" });

const std::string pgmconf_file_double_dot_relative_2 = pgmconf_generator({ "../../User/ws/product/pattern_test/**" });

const std::string pgmconf_file_double_dot_relative_3 =
    pgmconf_generator({ "../ws/product/../product/pattern_test/**" });

const std::string pgmconf_file_double_dot_absolute_wildcard =
    pgmconf_generator({ file_scheme_user_dir_encoded + "**/../product/pattern_test/**" });

const std::string pgmconf_file_double_dot_relative_wildcard = pgmconf_generator({ "../**/../product/pattern_test/**" });

const std::string pgmconf_file_root_asterisk = pgmconf_generator({ "*" });

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
    URI_3,
    URI_NON_STANDARD_0,
    URI_NON_STANDARD_1,
    URI_NON_STANDARD_2,
    COMBINATION,
    DOUBLE_DOT_ABSOLUTE,
    DOUBLE_DOT_RELATIVE_1,
    DOUBLE_DOT_RELATIVE_2,
    DOUBLE_DOT_RELATIVE_3,
    DOUBLE_DOT_ABSOLUTE_WILDCARD,
    DOUBLE_DOT_RELATIVE_WILDCARD
};

enum class pgmconf_variants
{
    RELATIVE_PLATFORM_DEPENDENT,
    RELATIVE_PLATFORM_INDEPENDENT,
    RELATIVE_WILD,
    ABSOLUTE,
    ABSOLUTE_WILD,
    URI_FULL,
    URI_WILD_01,
    URI_WILD_02,
    URI_WILD_03,
    URI_WILD_04,
    URI_WILD_05,
    URI_NON_STANDARD_0,
    URI_NON_STANDARD_1,
    URI_NON_STANDARD_2,
    DOUBLE_DOT_ABSOLUTE,
    DOUBLE_DOT_RELATIVE_1,
    DOUBLE_DOT_RELATIVE_2,
    DOUBLE_DOT_RELATIVE_3,
    DOUBLE_DOT_ABSOLUTE_WILDCARD,
    DOUBLE_DOT_RELATIVE_WILDCARD,
    ROOT_ASTERISK
};

const auto root_dir_loc = is_windows() ? resource_location("file:///c%3A/") : resource_location("file:///home/");
const auto user_dir_loc = resource_location::join(root_dir_loc, "User/");

const auto ws_loc = resource_location::join(user_dir_loc, "ws/");
const auto hlasmplugin_folder_loc(resource_location::join(ws_loc, ".hlasmplugin/"));
const auto proc_grps_loc(resource_location::join(hlasmplugin_folder_loc, "proc_grps.json"));
const auto pgm_conf_loc(resource_location::join(hlasmplugin_folder_loc, "pgm_conf.json"));

const auto root_source_file = resource_location::join(user_dir_loc, "ws/");

const auto product_loc = resource_location::join(ws_loc, "product/");
const auto pattern_test_dir_loc = resource_location::join(product_loc, "pattern_test/");
const auto pattern_est_dir_loc = resource_location::join(product_loc, "pattern_est/");
const auto patter_test_dir_loc = resource_location::join(product_loc, "patter_test/");

const auto pattern_test_source_loc(resource_location::join(pattern_test_dir_loc, "s%2525urce%3B")); // -> "s%25urce;"
const auto pattern_test_lib_loc(resource_location::join(pattern_test_dir_loc, "l%2525bs%3B/")); // -> "l%25bs;"
const auto pattern_test_lib_sublib1_loc(resource_location::join(pattern_test_lib_loc, "sublib1/"));
const auto pattern_test_lib_sublib2_loc(resource_location::join(pattern_test_lib_loc, "sublib2/"));
const auto pattern_test_macro1_loc(resource_location::join(pattern_test_lib_sublib1_loc, "mac1"));
const auto pattern_test_macro2_loc(resource_location::join(pattern_test_lib_sublib2_loc, "mac2"));

const auto temp_lib_loc = resource_location::join(root_dir_loc, "Temp/Lib/");
const auto temp_lib2_libs_loc =
    resource_location::join(root_dir_loc, "Temp/Lib2/L%2525bs%3B/"); // -> "Temp/Lib2/L%25bs;/"

const auto different_libs_loc = resource_location::join(ws_loc, "different_libs/");
const auto different_libs2_libs_loc = resource_location::join(ws_loc, "different_libs2/Libs/");

const auto different_libs2_libs_subdir = resource_location::join(different_libs2_libs_loc, "subdir/");
const auto temp_lib2_libs_subdir = resource_location::join(temp_lib2_libs_loc, "subdir/");

const char* pattern_lib_sublib1_abs_path = is_windows() ? "C:\\\\User\\ws\\product\\pattern_test\\l%25bs;\\sublib1\\"
                                                        : "/home/User/ws/product/pattern_test/l%25bs;/sublib1/";
const char* pattern_lib_sublib2_abs_path = is_windows() ? "c:\\\\User\\ws\\product\\pAttErn_test\\l%25bs;\\sublib2\\"
                                                        : "/home/User/ws/product/pattern_test/l%25bs;/sublib2/";

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
            case pgroup_variants::URI_3:
                return pgroups_file_pattern_uri_3;
            case pgroup_variants::URI_NON_STANDARD_0:
                return pgroups_file_pattern_uri_non_standard_0;
            case pgroup_variants::URI_NON_STANDARD_1:
                return pgroups_file_pattern_uri_non_standard_1;
            case pgroup_variants::URI_NON_STANDARD_2:
                return pgroups_file_pattern_uri_non_standard_2;
            case pgroup_variants::COMBINATION:
                return pgroups_file_pattern_combination;
            case pgroup_variants::DOUBLE_DOT_ABSOLUTE:
                return pgroups_file_pattern_double_dot_absolute;
            case pgroup_variants::DOUBLE_DOT_RELATIVE_1:
                return pgroups_file_pattern_double_dot_relative_1;
            case pgroup_variants::DOUBLE_DOT_RELATIVE_2:
                return pgroups_file_pattern_double_dot_relative_2;
            case pgroup_variants::DOUBLE_DOT_RELATIVE_3:
                return pgroups_file_pattern_double_dot_relative_3;
            case pgroup_variants::DOUBLE_DOT_ABSOLUTE_WILDCARD:
                return pgroups_file_pattern_double_dot_absolute_wildcard;
            case pgroup_variants::DOUBLE_DOT_RELATIVE_WILDCARD:
                return pgroups_file_pattern_double_dot_relative_wildcard;
        }
        throw std::logic_error("Not implemented");
    }

protected:
    std::string get_pgmconf_file(pgmconf_variants variant)
    {
        switch (variant)
        {
            case pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT:
                return pgmconf_file_relative_platform_dependent_slashes;
            case pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT:
                return pgmconf_file_relative_platform_independent_slashes;
            case pgmconf_variants::RELATIVE_WILD:
                return pgmconf_file_relative_wild;
            case pgmconf_variants::ABSOLUTE:
                return pgmconf_file_absolute;
            case pgmconf_variants::ABSOLUTE_WILD:
                return pgmconf_file_absolute_wild;
            case pgmconf_variants::URI_FULL:
                return pgmconf_file_uri_full;
            case pgmconf_variants::URI_WILD_01:
                return pgmconf_file_platform_uri_wild_01;
            case pgmconf_variants::URI_WILD_02:
                return pgmconf_file_platform_uri_wild_02;
            case pgmconf_variants::URI_WILD_03:
                return pgmconf_file_platform_uri_wild_03;
            case pgmconf_variants::URI_WILD_04:
                return pgmconf_file_platform_uri_wild_04;
            case pgmconf_variants::URI_WILD_05:
                return pgmconf_file_platform_uri_wild_05;
            case pgmconf_variants::URI_NON_STANDARD_0:
                return pgmconf_file_platform_file_scheme_non_standard_0;
            case pgmconf_variants::URI_NON_STANDARD_1:
                return pgmconf_file_platform_file_scheme_non_standard_1;
            case pgmconf_variants::URI_NON_STANDARD_2:
                return pgmconf_file_platform_file_scheme_non_standard_2;
            case pgmconf_variants::DOUBLE_DOT_ABSOLUTE:
                return pgmconf_file_double_dot_absolute;
            case pgmconf_variants::DOUBLE_DOT_RELATIVE_1:
                return pgmconf_file_double_dot_relative_1;
            case pgmconf_variants::DOUBLE_DOT_RELATIVE_2:
                return pgmconf_file_double_dot_relative_2;
            case pgmconf_variants::DOUBLE_DOT_RELATIVE_3:
                return pgmconf_file_double_dot_relative_3;
            case pgmconf_variants::DOUBLE_DOT_ABSOLUTE_WILDCARD:
                return pgmconf_file_double_dot_absolute_wildcard;
            case pgmconf_variants::DOUBLE_DOT_RELATIVE_WILDCARD:
                return pgmconf_file_double_dot_relative_wildcard;
            case pgmconf_variants::ROOT_ASTERISK:
                return pgmconf_file_root_asterisk;
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
        did_open_file(root_source_file, 1, source_txt);
    }

    MOCK_METHOD(hlasm_plugin::utils::value_task<list_directory_result>,
        list_directory_files,
        (const hlasm_plugin::utils::resource::resource_location& path),
        (const, override));

    std::string canonical(
        const hlasm_plugin::utils::resource::resource_location& res_loc, std::error_code&) const override
    {
        return res_loc.get_path();
    }

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_subdirs_and_symlinks(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == user_dir_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "ws", ws_loc } }, hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == ws_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "product_loc", product_loc } }, hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == product_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "pattern_est", pattern_est_dir_loc },
                      { "pattern_test", pattern_test_dir_loc },
                      { "patter_test", patter_test_dir_loc } },
                    hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == pattern_test_dir_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "libs", pattern_test_lib_loc } }, hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == pattern_test_lib_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { pattern_lib_sublib1_abs_path, pattern_test_lib_sublib1_loc },
                      { pattern_lib_sublib2_abs_path, pattern_test_lib_sublib2_loc } },
                    hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == different_libs2_libs_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "subdir", different_libs2_libs_subdir } }, hlasm_plugin::utils::path::list_directory_rc::done });

        if (location == temp_lib2_libs_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "subdir", temp_lib2_libs_subdir } }, hlasm_plugin::utils::path::list_directory_rc::done });

        return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
            { {}, hlasm_plugin::utils::path::list_directory_rc::done });
    }
};

struct test_variables_lib_pattern
{
    file_manager_lib_pattern file_manager;
    lib_config config;
    shared_json global_settings;
    workspace ws;

public:
    test_variables_lib_pattern(pgroup_variants pgroup_variant, pgmconf_variants pgmconf_variant)
        : file_manager(pgroup_variant, pgmconf_variant)
        , config()
        , global_settings(make_empty_shared_json())
        , ws(ws_loc, file_manager, config, global_settings)
    {
        ws.open().run();
    };
};

auto list_directory_cortn(list_directory_result result)
{
    return [result](const auto&) { return hlasm_plugin::utils::value_task<list_directory_result>::from_value(result); };
}

void verify_absolute(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::ABSOLUTE, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib2_libs_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib2_libs_subdir))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_relative_1(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::RELATIVE, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_relative_2(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::RELATIVE_2, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_relative_3(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::RELATIVE_3, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_dir_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(patter_test_dir_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_est_dir_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_uri_1(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::URI, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_uri_2_3(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::URI_2, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_uri_non_standard(pgroup_variants pgroup_variant, pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variant, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_combination(pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variants::COMBINATION, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib2_libs_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(temp_lib2_libs_subdir))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(different_libs_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(different_libs2_libs_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(different_libs2_libs_subdir))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_double_dot(pgroup_variants pgroup_variant, pgmconf_variants pgmconf_variant)
{
    test_variables_lib_pattern vars(pgroup_variant, pgmconf_variant);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(pattern_test_source_loc));
    parse_all_files(vars.ws);
}

void verify_root_asterisk()
{
    test_variables_lib_pattern vars(pgroup_variants::DOUBLE_DOT_ABSOLUTE, pgmconf_variants::ROOT_ASTERISK);

    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn({ {}, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib1_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac1", pattern_test_macro1_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));
    EXPECT_CALL(vars.file_manager, list_directory_files(pattern_test_lib_sublib2_loc))
        .WillOnce(::testing::Invoke(list_directory_cortn(
            { { { "mac2", pattern_test_macro2_loc } }, hlasm_plugin::utils::path::list_directory_rc::done })));

    run_if_valid(vars.ws.did_open_file(root_source_file));
    parse_all_files(vars.ws);
}
} // namespace
TEST(workspace_pattern_test, absolute) { verify_absolute(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, absolute_independent) { verify_absolute(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, relative_1) { verify_relative_1(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_1_independent)
{
    verify_relative_1(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, relative_2) { verify_relative_2(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_2_independent)
{
    verify_relative_2(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, relative_3) { verify_relative_3(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, relative_3_independent)
{
    verify_relative_3(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, uri_1) { verify_uri_1(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, uri_1_independent) { verify_uri_1(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_2) { verify_uri_2_3(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, uri_2_independent) { verify_uri_2_3(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_3) { verify_uri_2_3(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, uri_3_independent) { verify_uri_2_3(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT); }

TEST(workspace_pattern_test, uri_non_standard_0)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_0, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_0_independent)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_0, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_1)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_1, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_1_independent)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_1, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_2)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_2, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, uri_non_standard_2_independent)
{
    if (is_windows())
        verify_uri_non_standard(pgroup_variants::URI_NON_STANDARD_2, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}
TEST(workspace_pattern_test, combination) { verify_combination(pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT); }

TEST(workspace_pattern_test, combination_independent)
{
    verify_combination(pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, double_dot_absolute)
{
    verify_double_dot(pgroup_variants::DOUBLE_DOT_ABSOLUTE, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, double_dot_relative_1)
{
    verify_double_dot(pgroup_variants::DOUBLE_DOT_RELATIVE_1, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, double_dot_relative_2)
{
    verify_double_dot(pgroup_variants::DOUBLE_DOT_RELATIVE_2, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, double_dot_relative_3)
{
    verify_double_dot(pgroup_variants::DOUBLE_DOT_RELATIVE_3, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

// TODO - make the following pattern work: "**/.."
// TEST(workspace_pattern_test, double_dot_absolute_wildcard)
//{
//    verify_double_dot(pgroup_variants::DOUBLE_DOT_ABSOLUTE_WILDCARD, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
//}
//
// TEST(workspace_pattern_test, double_dot_relative_wildcard)
//{
//    verify_double_dot(pgroup_variants::DOUBLE_DOT_RELATIVE_WILDCARD, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
//}

TEST(workspace_pattern_test, pgm_conf_relative_wild) { verify_combination(pgmconf_variants::RELATIVE_WILD); }

TEST(workspace_pattern_test, pgm_conf_absolute) { verify_combination(pgmconf_variants::ABSOLUTE); }

TEST(workspace_pattern_test, pgm_conf_absolute_wild) { verify_combination(pgmconf_variants::ABSOLUTE_WILD); }

TEST(workspace_pattern_test, pgm_conf_uri_full) { verify_combination(pgmconf_variants::URI_FULL); }

TEST(workspace_pattern_test, pgm_conf_uri_wild_01) { verify_combination(pgmconf_variants::URI_WILD_01); }

TEST(workspace_pattern_test, pgm_conf_uri_wild_02) { verify_combination(pgmconf_variants::URI_WILD_02); }

TEST(workspace_pattern_test, pgm_conf_uri_wild_03) { verify_combination(pgmconf_variants::URI_WILD_03); }

TEST(workspace_pattern_test, pgm_conf_uri_wild_04) { verify_combination(pgmconf_variants::URI_WILD_04); }

TEST(workspace_pattern_test, pgm_conf_uri_wild_05) { verify_combination(pgmconf_variants::URI_WILD_05); }

TEST(workspace_pattern_test, pgm_conf_uri_non_standard_0)
{
    if (is_windows())
        verify_combination(pgmconf_variants::URI_NON_STANDARD_0);
}

TEST(workspace_pattern_test, pgm_conf_uri_non_standard_1)
{
    if (is_windows())
        verify_combination(pgmconf_variants::URI_NON_STANDARD_1);
}

TEST(workspace_pattern_test, pgm_conf_uri_non_standard_2)
{
    if (is_windows())
        verify_combination(pgmconf_variants::URI_NON_STANDARD_2);
}

TEST(workspace_pattern_test, pgm_conf_double_dot_absolute)
{
    verify_combination(pgmconf_variants::DOUBLE_DOT_ABSOLUTE);
}

TEST(workspace_pattern_test, pgm_conf_double_dot_relative_1)
{
    verify_combination(pgmconf_variants::DOUBLE_DOT_RELATIVE_1);
}

TEST(workspace_pattern_test, pgm_conf_double_dot_relative_2)
{
    verify_combination(pgmconf_variants::DOUBLE_DOT_RELATIVE_2);
}

TEST(workspace_pattern_test, pgm_conf_double_dot_relative_3)
{
    verify_combination(pgmconf_variants::DOUBLE_DOT_RELATIVE_3);
}

// TODO - make the following pattern work: "**/.."
// TEST(workspace_pattern_test, pgm_conf_double_dot_absolute_wildcard)
//{
//    verify_combination(pgmconf_variants::DOUBLE_DOT_ABSOLUTE_WILDCARD);
//}
//
// TEST(workspace_pattern_test, pgm_conf_double_dot_relative_wildcard)
//{
//    verify_combination(pgmconf_variants::DOUBLE_DOT_RELATIVE_WILDCARD);
//}

TEST(workspace_pattern_test, pgm_root_asterisk) { verify_root_asterisk(); }

namespace {
const std::string pgroups_file_inf = pgroups_generator({ file_scheme_user_dir_encoded + "ws/symlinks/inf0/**" });
const std::string pgroups_file_inf_2 = pgroups_generator({ file_scheme_user_dir_encoded + "ws/symlinks/inf/**" });
const std::string pgroups_file_canonical = pgroups_generator({ file_scheme_user_dir_encoded + "ws/canonical/**" });

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

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_subdirs_and_symlinks(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location.get_uri().ends_with("/inf") || location.get_uri().ends_with("/inf/"))
        {
            // Just append a dir and return
            auto new_loc = resource_location::join(location, "inf");
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { new_loc.get_uri(), new_loc } }, hlasm_plugin::utils::path::list_directory_rc::done });
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
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
                {
                    { inf1.get_uri(), inf1 },
                    { inf2.get_uri(), inf2 },
                    { inf3.get_uri(), inf3 },
                    { inf4.get_uri(), inf4 },
                    { inf5.get_uri(), inf5 },
                },
                hlasm_plugin::utils::path::list_directory_rc::done,
            });
        }

        if (location.get_uri().ends_with("/canonical/") || location.get_uri().ends_with("/canonical"))
        {
            auto can = resource_location::join(location, "can1");
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "can1", can } }, hlasm_plugin::utils::path::list_directory_rc::done });
        }

        if (location.get_uri().ends_with("/can1"))
        {
            auto can = resource_location::join(location, "can2");
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "can2", can } }, hlasm_plugin::utils::path::list_directory_rc::done });
        }

        if (location.get_uri().ends_with("/can2"))
        {
            auto can = resource_location::join(location, "can3");
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "can3", can } }, hlasm_plugin::utils::path::list_directory_rc::done });
        }

        if (location.get_uri().ends_with("/can3"))
        {
            auto can = resource_location::join(location, "canonical");
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
                { { { "canonical", can } }, hlasm_plugin::utils::path::list_directory_rc::done });
        }


        return hlasm_plugin::utils::value_task<list_directory_result>::from_value(
            { {}, hlasm_plugin::utils::path::list_directory_rc::done });
    }
};

void verify_infinit_loop(pgroup_symlinks_variants pgroup_variant, pgmconf_variants pgmconf_variant)
{
    ::testing::NiceMock<file_manager_infinit_loop> file_manager(pgroup_variant, pgmconf_variant);
    lib_config config;
    shared_json global_settings = make_empty_shared_json();

    EXPECT_CALL(file_manager, list_directory_files)
        .WillRepeatedly(::testing::Invoke([](auto) -> hlasm_plugin::utils::value_task<list_directory_result> {
            co_return { {}, hlasm_plugin::utils::path::list_directory_rc::done };
        }));

    workspace ws(ws_loc, file_manager, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(pattern_test_source_loc));
    parse_all_files(ws);

    // No explicit expectations - it is just expected that this will not crash or end up in a loop.
}

} // namespace

TEST(workspace_pattern_test, infinit_loop_general)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_2)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT_2, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_general_2_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::INFINIT_2, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_canonical)
{
    verify_infinit_loop(pgroup_symlinks_variants::CANONICAL, pgmconf_variants::RELATIVE_PLATFORM_DEPENDENT);
}

TEST(workspace_pattern_test, infinit_loop_canonical_independent)
{
    verify_infinit_loop(pgroup_symlinks_variants::CANONICAL, pgmconf_variants::RELATIVE_PLATFORM_INDEPENDENT);
}
