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

#include <algorithm>
#include <iterator>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "empty_configs.h"
#include "lsp/completion_item.h"
#include "nlohmann/json.hpp"
#include "semantics/highlighting_info.h"
#include "utils/content_loader.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/library_local.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using hlasm_plugin::utils::platform::is_windows;
using namespace hlasm_plugin::utils::resource;

namespace {
const auto proc_grps_loc = resource_location("proc_grps.json");
const auto file_loc = resource_location("test_uri");
const auto users_dir =
    is_windows() ? resource_location("file:///c%3A/Users/") : resource_location("file:///home/user/");

const auto ws_loc = resource_location::join(users_dir, "ws/");

const auto pgm1_loc = resource_location::join(ws_loc, "pgm1");
const auto pgm_override_loc = resource_location::join(ws_loc, "pgm_override");
const auto pgm_anything_loc = resource_location::join(ws_loc, "pgms/anything");
const auto pgm_outside_ws = resource_location::join(users_dir, "outside/anything");
} // namespace

const std::string file_proc_grps_content = is_windows() ?
                                                        R"({
    "pgroups": [
        {
            "name": "P1",
            "libs": [
                "C:\\Users\\Desktop\\ASLib",
                "lib",
                "libs\\lib2\\",
                "file:///c%3A/Users/Desktop/Temp/",
                ""
            ],
            "asm_options": {
                "SYSPARM": "SEVEN",
                "PROFILE": "MAC1"
            },
            "preprocessor": "DB2"
        },
        {
            "name": "P2",
            "libs": [
                "C:\\Users\\Desktop\\ASLib",
                "P2lib",
                "P2libs\\libb"
            ]
        }
    ]
})"
                                                        : R"({
    "pgroups": [
        {
            "name": "P1",
            "libs": [
                "/home/user/ASLib",
                "lib",
                "libs/lib2/",
                "file:///home/user/Temp/",
                ""
            ],
            "asm_options": {
                "SYSPARM": "SEVEN",
                "PROFILE": "MAC1"
            },
            "preprocessor": "DB2"
        },
        {
            "name": "P2",
            "libs": [
                "/home/user/ASLib",
                "P2lib",
                "P2libs/libb"
            ]
        }
    ]
})";

const std::string file_pgm_conf_content = is_windows() ? R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgm_override",
      "pgroup": "P1",
      "asm_options":
      {
        "PROFILE": "PROFILE OVERRIDE"
      }
    },
    {
      "program": "pgms\\*",
      "pgroup": "P2"
    }
  ]
})"
                                                       : R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgm_override",
      "pgroup": "P1",
      "asm_options":
      {
        "PROFILE": "PROFILE OVERRIDE"
      }
    },
    {
      "program": "pgms/*",
      "pgroup": "P2"
    }
  ]
})";

class file_manager_proc_grps_test : public file_manager_impl
{
public:
    hlasm_plugin::utils::value_task<std::optional<std::string>> get_file_content(
        const resource_location& location) override
    {
        using hlasm_plugin::utils::value_task;
        if (hlasm_plugin::utils::resource::filename(location) == "proc_grps.json")
            return value_task<std::optional<std::string>>::from_value(file_proc_grps_content);
        else if (hlasm_plugin::utils::resource::filename(location) == "pgm_conf.json")
            return value_task<std::optional<std::string>>::from_value(file_pgm_conf_content);
        else
            return value_task<std::optional<std::string>>::from_value(std::nullopt);
    }

    // Inherited via file_manager
    file_content_state did_open_file(const resource_location&, version_t, std::string) override
    {
        return file_content_state::changed_content;
    }
    void did_change_file(const resource_location&, version_t, const document_change*, size_t) override {}
    void did_close_file(const resource_location&) override {}
};

void check_process_group(const processor_group& pg, std::span<resource_location> expected)
{
    EXPECT_EQ(std::size(expected), pg.libraries().size()) << "For pg.name() = " << pg.name();
    for (size_t i = 0; i < std::min(std::size(expected), pg.libraries().size()); ++i)
    {
        library_local* libl = dynamic_cast<library_local*>(pg.libraries()[i].get());
        ASSERT_NE(libl, nullptr);
        EXPECT_EQ(expected[i], libl->get_location())
            << "Expected: " << expected[i].get_uri() << "\n Got: " << libl->get_location().get_uri()
            << "\n For pg.name() = " << pg.name() << " and i = " << i;
    }
}

TEST(workspace, load_config_synthetic)
{
    file_manager_proc_grps_test file_manager;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(ws_loc, file_manager, config, global_settings);

    ws.open().run();

    // Check P1
    auto& pg = ws.get_proc_grp(basic_conf { "P1" });
    EXPECT_EQ("P1", pg.name());
    auto expected = []() -> std::array<resource_location, 5> {
        if (is_windows())
            return { resource_location("file:///c%3A/Users/Desktop/ASLib/"),
                resource_location("file:///c%3A/Users/ws/lib/"),
                resource_location("file:///c%3A/Users/ws/libs/lib2/"),
                resource_location("file:///c%3A/Users/Desktop/Temp/"),
                resource_location("file:///c%3A/Users/ws/") };
        else
            return { resource_location("file:///home/user/ASLib/"),
                resource_location("file:///home/user/ws/lib/"),
                resource_location("file:///home/user/ws/libs/lib2/"),
                resource_location("file:///home/user/Temp/"),
                resource_location("file:///home/user/ws/") };
    }();
    check_process_group(pg, expected);

    // Check P2
    auto& pg2 = ws.get_proc_grp(basic_conf { "P2" });
    EXPECT_EQ("P2", pg2.name());

    auto expected2 = []() -> std::array<resource_location, 3> {
        if (is_windows())
            return { resource_location("file:///c%3A/Users/Desktop/ASLib/"),
                resource_location("file:///c%3A/Users/ws/P2lib/"),
                resource_location("file:///c%3A/Users/ws/P2libs/libb/") };
        else
            return { resource_location("file:///home/user/ASLib/"),
                resource_location("file:///home/user/ws/P2lib/"),
                resource_location("file:///home/user/ws/P2libs/libb/") };
    }();
    check_process_group(pg2, expected2);

    // Check PGM1
    // test of pgm_conf and workspace::get_proc_grp
    auto& pg3 = ws.get_proc_grp(pgm1_loc);
    check_process_group(pg3, expected);

    // Check PGM anything
    auto& pg4 = ws.get_proc_grp(pgm_anything_loc);
    check_process_group(pg4, expected2);

    // test of asm_options
    const auto& asm_options = ws.get_asm_options(pgm1_loc);
    EXPECT_EQ("SEVEN", asm_options.sysparm);
    EXPECT_EQ("MAC1", asm_options.profile);

    const auto& pp_options = ws.get_preprocessor_options(pgm1_loc);
    EXPECT_TRUE(pp_options.size() == 1 && std::holds_alternative<db2_preprocessor_options>(pp_options.front()));

    // test of asm_options override
    const auto& asm_options_override = ws.get_asm_options(pgm_override_loc);
    EXPECT_EQ("SEVEN", asm_options_override.sysparm);
    EXPECT_EQ("PROFILE OVERRIDE", asm_options_override.profile);

    // test sysin options in workspace
    const auto& asm_options_ws = ws.get_asm_options(pgm_anything_loc);
    EXPECT_EQ(asm_options_ws.sysin_dsn, "pgms");
    EXPECT_EQ(asm_options_ws.sysin_member, "anything");

    // test sysin options out of workspace
    const auto& asm_options_ows = ws.get_asm_options(pgm_outside_ws);
    EXPECT_EQ(asm_options_ows.sysin_dsn, is_windows() ? "c:\\Users\\outside" : "/home/user/outside");
    EXPECT_EQ(asm_options_ows.sysin_member, "anything");
}

TEST(workspace, pgm_conf_malformed)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, R"({ "pgms": [})");
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();

    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0003" }));
}

TEST(workspace, proc_grps_malformed)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, R"({ "pgroups" []})");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();

    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0002" }));
}

TEST(workspace, pgm_conf_missing)
{
    file_manager_impl fm;
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();

    ws.collect_diags();
    EXPECT_EQ(ws.diags().size(), 0U);
}

TEST(workspace, proc_grps_missing)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();

    ws.collect_diags();
    EXPECT_EQ(ws.diags().size(), 0U);
}

TEST(workspace, pgm_conf_noproc_proc_group)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, R"({
  "pgms": [
    {
      "program": "temp.hlasm",
      "pgroup": "*NOPROC*"
    }
  ]
})");
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);
    const auto temp_hlasm = resource_location::join(empty_ws, "temp.hlasm");
    fm.did_open_file(temp_hlasm, 1, "");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(temp_hlasm));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_EQ(ws.diags().size(), 0U);
}

TEST(workspace, pgm_conf_unknown_proc_group)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, R"({
  "pgms": [
    {
      "program": "temp.hlasm",
      "pgroup": "UNKNOWN"
    }
  ]
})");
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);
    const auto temp_hlasm = resource_location::join(empty_ws, "temp.hlasm");
    fm.did_open_file(temp_hlasm, 1, "");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(temp_hlasm));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0004" }));
}

TEST(workspace, missing_proc_group_diags)
{
    file_manager_impl fm;
    const auto pgm_conf_ws_loc = resource_location::join(ws_loc, pgm_conf_name);
    const auto proc_grps_ws_loc = resource_location::join(ws_loc, proc_grps_name);
    const auto pgm1_wildcard_loc = resource_location::join(ws_loc, "pgms/pgm1");
    const auto pgm1_different_loc = resource_location::join(ws_loc, "different/pgm1");
    fm.did_open_file(pgm_conf_ws_loc, 0, file_pgm_conf_content);
    fm.did_open_file(proc_grps_ws_loc, 0, empty_proc_grps);
    fm.did_open_file(pgm1_loc, 1, "");
    fm.did_open_file(pgm1_wildcard_loc, 1, "");
    fm.did_open_file(pgm1_different_loc, 1, "");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(ws_loc, fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0004" }));

    ws.include_advisory_configuration_diagnostics(true);
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0004", "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    ws.include_advisory_configuration_diagnostics(false);
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    run_if_valid(ws.did_open_file(pgm1_wildcard_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0004" }));

    run_if_valid(ws.did_close_file(pgm1_wildcard_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    run_if_valid(ws.did_open_file(pgm1_different_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());
}

TEST(workspace, missing_proc_group_diags_wildcards)
{
    file_manager_impl fm;
    const auto pgm_conf_ws_loc = resource_location::join(ws_loc, pgm_conf_name);
    const auto proc_grps_ws_loc = resource_location::join(ws_loc, proc_grps_name);
    const auto pgm1_wildcard_loc = resource_location::join(ws_loc, "pgms/pgm1");
    const auto pgm1_different_loc = resource_location::join(ws_loc, "different/pgm1");
    fm.did_open_file(
        pgm_conf_ws_loc, 0, R"({"pgms":[{"program": "pgm1","pgroup": "P1"},{"program": "pgm*","pgroup": "P2"}]})");
    fm.did_open_file(proc_grps_ws_loc, 0, R"({"pgroups":[{"name":"P1","libs":[]}]})");
    fm.did_open_file(pgm1_loc, 1, "");
    fm.did_open_file(pgm1_wildcard_loc, 1, "");
    fm.did_open_file(pgm1_different_loc, 1, "");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(ws_loc, fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    ws.include_advisory_configuration_diagnostics(true);
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());
}

TEST(workspace, missing_proc_group_diags_wildcards_noproc)
{
    file_manager_impl fm;
    const auto pgm_conf_ws_loc = resource_location::join(ws_loc, pgm_conf_name);
    const auto proc_grps_ws_loc = resource_location::join(ws_loc, proc_grps_name);
    const auto pgm1_wildcard_loc = resource_location::join(ws_loc, "pgms/pgm1");
    const auto pgm1_different_loc = resource_location::join(ws_loc, "different/pgm1");
    fm.did_open_file(pgm_conf_ws_loc,
        0,
        R"({"pgms":[{"program": "pgm1","pgroup": "*NOPROC*"},{"program": "pgm*","pgroup": "P2"}]})");
    fm.did_open_file(proc_grps_ws_loc, 0, empty_proc_grps);
    fm.did_open_file(pgm1_loc, 1, "");
    fm.did_open_file(pgm1_wildcard_loc, 1, "");
    fm.did_open_file(pgm1_different_loc, 1, "");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(ws_loc, fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    ws.include_advisory_configuration_diagnostics(true);
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());
}

TEST(workspace, asm_options_invalid)
{
    std::string proc_file = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "lib" ],    
      "asm_options": {
        "SYSPARM" : 42
   
        }
    }
  ]
})";
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, proc_file);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();

    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0002" }));
}

class file_manager_asm_test : public file_manager_proc_grps_test
{
public:
    hlasm_plugin::utils::value_task<std::optional<std::string>> get_file_content(
        const resource_location& location) override
    {
        if (hlasm_plugin::utils::resource::filename(location) == "proc_grps.json")
            return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [],
      "asm_options": {
         "GOFF":true,
         "XOBJECT":true
      }
    }
  ]
})");
        else
            return file_manager_proc_grps_test::get_file_content(location);
    }
};

TEST(workspace, asm_options_goff_xobject_redefinition)
{
    file_manager_asm_test file_manager;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(ws_loc, file_manager, config, global_settings);

    ws.open().run();

    ws.collect_diags();
    EXPECT_TRUE(contains_message_codes(ws.diags(), { "W0002" }));
}

TEST(workspace, proc_grps_with_substitutions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name,
        0,
        R"({ "pgroups":[{"name":"a${config:name}b","libs":["${config:lib1}","${config:lib2}"]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"name":"proc_group","lib1":"library1","lib2":"library2"})"));
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();
    ws.collect_diags();

    EXPECT_TRUE(ws.diags().empty());

    const auto& pg = ws.get_proc_grp(basic_conf { "aproc_groupb" });

    using hlasm_plugin::utils::resource::resource_location;

    ASSERT_EQ(pg.libraries().size(), 2);
    EXPECT_EQ(dynamic_cast<const library_local*>(pg.libraries()[0].get())->get_location(),
        resource_location::join(empty_ws, "library1/"));
    EXPECT_EQ(dynamic_cast<const library_local*>(pg.libraries()[1].get())->get_location(),
        resource_location::join(empty_ws, "library2/"));
}

TEST(workspace, pgm_conf_with_substitutions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name,
        0,
        R"({"pgms":[{"program":"test/${config:pgm_mask.0}","pgroup":"P1","asm_options":{"SYSPARM":"${config:sysparm}${config:sysparm}"}}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name": "P1","libs":[]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"pgm_mask":["file_name"],"sysparm":"DEBUG"})"));
    workspace ws(empty_ws, fm, config, global_settings);
    const auto test_loc = resource_location::join(empty_ws, "test");
    ws.open().run();
    ws.collect_diags();

    EXPECT_TRUE(ws.diags().empty());

    using hlasm_plugin::utils::resource::resource_location;

    const auto& opts = ws.get_asm_options(resource_location::join(test_loc, "file_name"));

    EXPECT_EQ(opts.sysparm, "DEBUGDEBUG");
}

TEST(workspace, missing_substitutions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, R"({"pgms":[{"program":"test/${config:pgm_mask}","pgroup":"P1"}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name":"P1","libs":["${config:lib}"]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(nlohmann::json::object());
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();
    ws.collect_diags();

    EXPECT_TRUE(matches_message_codes(ws.diags(), { "W0007", "W0007" }));
}

TEST(workspace, refresh_settings)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name,
        0,
        R"({"pgms":[{"program":"test/${config:pgm_mask.0}","pgroup":"P1","asm_options":{"SYSPARM":"${config:sysparm}${config:sysparm}"}}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name": "P1","libs":[]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"pgm_mask":["file_name"],"sysparm":"DEBUG"})"));
    workspace ws(empty_ws, fm, config, global_settings);
    const auto test_loc = resource_location::join(empty_ws, "test");
    ws.open().run();
    ws.collect_diags();

    EXPECT_TRUE(ws.diags().empty());

    using hlasm_plugin::utils::resource::resource_location;

    EXPECT_EQ(ws.get_asm_options(resource_location::join(test_loc, "file_name")).sysparm, "DEBUGDEBUG");
    EXPECT_FALSE(ws.settings_updated().run().value());

    global_settings = std::make_shared<const nlohmann::json>(
        nlohmann::json::parse(R"({"pgm_mask":["different_file"],"sysparm":"RELEASE"})"));
    EXPECT_TRUE(ws.settings_updated().run().value());

    EXPECT_EQ(ws.get_asm_options(resource_location::join(test_loc, "file_name")).sysparm, "");
    EXPECT_EQ(ws.get_asm_options(resource_location::join(test_loc, "different_file")).sysparm, "RELEASERELEASE");
}

TEST(workspace, opcode_suggestions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, R"({"pgms":[{"program":"pgm","pgroup":"P1"}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name": "P1","libs":[]}]})");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_ws, fm, config, global_settings);
    ws.open().run();
    ws.collect_diags();

    EXPECT_TRUE(ws.diags().empty());

    using hlasm_plugin::utils::resource::resource_location;
    std::vector<std::pair<std::string, size_t>> expected { { "LHI", 3 } };

    EXPECT_EQ(ws.make_opcode_suggestion(resource_location::join(empty_ws, "pgm"), "LHIXXX", false), expected);
    EXPECT_EQ(ws.make_opcode_suggestion(resource_location::join(empty_ws, "pgm_implicit"), "LHIXXX", false), expected);
}

TEST(workspace, lsp_file_not_processed_yet)
{
    file_manager_impl mngr;
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace ws(mngr, config, global_settings);
    ws.open().run();

    mngr.did_open_file(file_loc, 0, " LR 1,1");

    EXPECT_EQ(ws.definition(file_loc, { 0, 5 }), location({ 0, 5 }, file_loc));
    EXPECT_EQ(ws.references(file_loc, { 0, 5 }), location_list());
    EXPECT_EQ(ws.hover(file_loc, { 0, 5 }), "");
    EXPECT_EQ(ws.completion(file_loc, { 0, 5 }, '\0', completion_trigger_kind::invoked), lsp::completion_list_s());

    run_if_valid(ws.did_open_file(file_loc));
    // parsing not done yet

    EXPECT_EQ(ws.definition(file_loc, { 0, 5 }), location({ 0, 5 }, file_loc));
    EXPECT_EQ(ws.references(file_loc, { 0, 5 }), location_list());
    EXPECT_EQ(ws.hover(file_loc, { 0, 5 }), "");
    EXPECT_EQ(ws.completion(file_loc, { 0, 5 }, '\0', completion_trigger_kind::invoked), lsp::completion_list_s());

    // Prior to parsing, it should return default values
    EXPECT_EQ(ws.semantic_tokens(file_loc), semantics::lines_info());
    EXPECT_EQ(ws.last_metrics(file_loc), performance_metrics());
}
