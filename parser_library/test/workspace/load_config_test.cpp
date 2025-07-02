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
#include "completion_item.h"
#include "completion_trigger_kind.h"
#include "empty_configs.h"
#include "nlohmann/json.hpp"
#include "semantics/highlighting_info.h"
#include "utils/content_loader.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/library_local.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

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

std::vector<diagnostic> extract_diags(workspace& ws, workspace_configuration& cfg, bool advisory = false)
{
    std::vector<diagnostic> result;
    cfg.produce_diagnostics(result, ws.report_used_configuration_files(), advisory);
    ws.produce_diagnostics(result);
    return result;
}
} // namespace

TEST(workspace, pgm_conf_malformed)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, R"({ "pgms": [})");
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0003" }));
}

TEST(workspace, proc_grps_malformed)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, R"({ "pgroups" []})");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0002" }));
}

TEST(workspace, pgm_conf_missing)
{
    file_manager_impl fm;
    fm.did_open_file(empty_proc_grps_name, 0, empty_proc_grps);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), 0U);
}

TEST(workspace, proc_grps_missing)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), 0U);
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
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(temp_hlasm));
    parse_all_files(ws);

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), 0U);
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
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(temp_hlasm));
    parse_all_files(ws);

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0004" }));
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
    workspace_configuration ws_cfg(fm, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0004" }));

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg, true), { "W0004", "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    EXPECT_TRUE(extract_diags(ws, ws_cfg, true).empty());

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    run_if_valid(ws.did_open_file(pgm1_wildcard_loc));
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0004" }));

    run_if_valid(ws.did_close_file(pgm1_wildcard_loc));
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    run_if_valid(ws.did_open_file(pgm1_different_loc));
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
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
    workspace_configuration ws_cfg(fm, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg, true), { "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    EXPECT_TRUE(extract_diags(ws, ws_cfg, true).empty());
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
    workspace_configuration ws_cfg(fm, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(pgm1_loc));
    parse_all_files(ws);

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg, true), { "W0008" }));

    run_if_valid(ws.did_close_file(pgm1_loc));
    EXPECT_TRUE(extract_diags(ws, ws_cfg, true).empty());
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
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0002" }));
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
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    const auto& pg = ws_cfg.get_proc_grp(basic_conf { "aproc_groupb" });

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
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    const auto test_loc = resource_location::join(empty_ws, "test");
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    using hlasm_plugin::utils::resource::resource_location;

    auto [cfg, _] = ws_cfg.get_analyzer_configuration(resource_location::join(test_loc, "file_name")).run().value();

    EXPECT_EQ(cfg.opts.sysparm, "DEBUGDEBUG");
}

TEST(workspace, missing_substitutions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, R"({"pgms":[{"program":"test/${config:pgm_mask}","pgroup":"P1"}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name":"P1","libs":["${config:lib}"]}]})");

    lib_config config;
    shared_json global_settings = std::make_shared<const nlohmann::json>(nlohmann::json::object());
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "W0007", "W0007" }));
}

TEST(workspace, opcode_suggestions)
{
    file_manager_impl fm;

    fm.did_open_file(empty_pgm_conf_name, 0, R"({"pgms":[{"program":"pgm","pgroup":"P1"}]})");
    fm.did_open_file(empty_proc_grps_name, 0, R"({"pgroups":[{"name": "P1","libs":[]}]})");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

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
    workspace_configuration ws_cfg(mngr, resource_location(), global_settings, config, nullptr, nullptr);
    workspace ws(mngr, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    mngr.did_open_file(file_loc, 0, " LR 1,1");

    static const std::vector<completion_item> empty_list;

    EXPECT_EQ(ws.definition(file_loc, { 0, 5 }), location({ 0, 5 }, file_loc));
    EXPECT_EQ(ws.references(file_loc, { 0, 5 }), std::vector<location>());
    EXPECT_EQ(ws.hover(file_loc, { 0, 5 }), "");
    EXPECT_EQ(ws.completion(file_loc, { 0, 5 }, '\0', completion_trigger_kind::invoked), empty_list);

    run_if_valid(ws.did_open_file(file_loc));
    // parsing not done yet

    EXPECT_EQ(ws.definition(file_loc, { 0, 5 }), location({ 0, 5 }, file_loc));
    EXPECT_EQ(ws.references(file_loc, { 0, 5 }), std::vector<location>());
    EXPECT_EQ(ws.hover(file_loc, { 0, 5 }), "");
    EXPECT_EQ(ws.completion(file_loc, { 0, 5 }, '\0', completion_trigger_kind::invoked), empty_list);

    // Prior to parsing, it should return default values
    EXPECT_EQ(ws.semantic_tokens(file_loc), semantics::lines_info());
    EXPECT_EQ(ws.last_metrics(file_loc), performance_metrics());
}
