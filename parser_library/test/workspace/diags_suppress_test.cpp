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
#include "../message_consumer_mock.h"
#include "../workspace/empty_configs.h"
#include "empty_configs.h"
#include "lib_config.h"
#include "nlohmann/json.hpp"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

using namespace nlohmann;
using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;

std::string one_proc_grps = R"(
{
    "pgroups": [
        { "name": "P1", "libs": [] }
    ]
}
)";

const auto file_loc = resource_location("a_file");

std::vector<diagnostic> extract_diags(workspace& ws, workspace_configuration& cfg)
{
    std::vector<diagnostic> result;
    cfg.produce_diagnostics(result, {}, {});
    ws.produce_diagnostics(result);
    return result;
}

TEST(diags_suppress, no_suppress)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();

    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), 6U);
}

TEST(diags_suppress, do_suppress)
{
    lib_config config { .diag_supress_limit = 5 };
    shared_json global_settings = make_empty_shared_json();

    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    message_consumer_mock msg_consumer;

    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr);
    workspace ws(fm, ws_cfg);
    ws.set_message_consumer(&msg_consumer);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "SUP" }));
    EXPECT_TRUE(msg_consumer.messages.empty());
}

TEST(diags_suppress, pgm_supress_limit_changed)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    lib_config config;
    shared_json global_settings = make_empty_shared_json();

    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), 6U);

    std::string new_limit_str = R"("diagnosticsSuppressLimit":5,)";
    document_change ch(range({ 0, 1 }, { 0, 1 }), new_limit_str);

    fm.did_change_file(empty_pgm_conf_name, 1, std::span(&ch, 1));
    run_if_valid(ws_cfg.parse_configuration_file(empty_pgm_conf_name).then([&ws](auto result) {
        if (result == parse_config_file_result::parsed)
            ws.mark_all_opened_files();
    }));
    parse_all_files(ws);

    run_if_valid(ws.mark_file_for_parsing(file_loc, file_content_state::changed_content));
    parse_all_files(ws);

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "SUP" }));
}

TEST(diags_suppress, mark_for_parsing_only)
{
    file_manager_impl fm;
    fm.did_open_file(empty_pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(empty_proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    lib_config config { .diag_supress_limit = 5 };
    shared_json global_settings = make_empty_shared_json();

    workspace_configuration ws_cfg(fm, empty_ws, global_settings, config, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(file_loc));
    // parsing not done yet

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    parse_all_files(ws);

    EXPECT_FALSE(extract_diags(ws, ws_cfg).empty());
}
