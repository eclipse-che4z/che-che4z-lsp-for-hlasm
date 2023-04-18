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

TEST(diags_suppress, no_suppress)
{
    file_manager_impl fm;
    fm.did_open_file(pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(proc_grps_name, 0, one_proc_grps);

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

    workspace ws(fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    ws.collect_diags();

    EXPECT_EQ(ws.diags().size(), 6U);
}

TEST(diags_suppress, do_suppress)
{
    auto config = lib_config::load_from_json(R"({"diagnosticsSuppressLimit":5})"_json);
    shared_json global_settings = make_empty_shared_json();

    file_manager_impl fm;
    fm.did_open_file(pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    message_consumer_mock msg_consumer;

    workspace ws(fm, config, global_settings);
    ws.set_message_consumer(&msg_consumer);
    ws.open().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    ws.collect_diags();

    EXPECT_TRUE(matches_message_codes(ws.diags(), { "SUP" }));
    EXPECT_TRUE(msg_consumer.messages.empty());
}

TEST(diags_suppress, pgm_supress_limit_changed)
{
    file_manager_impl fm;
    fm.did_open_file(pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(proc_grps_name, 0, one_proc_grps);

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

    workspace ws(fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(file_loc));
    parse_all_files(ws);

    ws.collect_diags();
    EXPECT_EQ(ws.diags().size(), 6U);

    std::string new_limit_str = R"("diagnosticsSuppressLimit":5,)";
    document_change ch(range({ 0, 1 }, { 0, 1 }), new_limit_str.c_str(), new_limit_str.size());

    fm.did_change_file(pgm_conf_name, 1, &ch, 1);
    run_if_valid(ws.did_change_file(pgm_conf_name, file_content_state::changed_content));
    parse_all_files(ws);

    run_if_valid(ws.did_change_file(file_loc, file_content_state::changed_content));
    parse_all_files(ws);

    ws.diags().clear();
    ws.collect_diags();
    EXPECT_TRUE(matches_message_codes(ws.diags(), { "SUP" }));
}

TEST(diags_suppress, mark_for_parsing_only)
{
    file_manager_impl fm;
    fm.did_open_file(pgm_conf_name, 0, empty_pgm_conf);
    fm.did_open_file(proc_grps_name, 0, one_proc_grps);

    fm.did_open_file(file_loc, 0, R"(
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
    LR 1,
)");

    auto config = lib_config::load_from_json(R"({"diagnosticsSuppressLimit":5})"_json);
    shared_json global_settings = make_empty_shared_json();

    workspace ws(fm, config, global_settings);
    ws.open().run();
    run_if_valid(ws.did_open_file(file_loc));
    // parsing not done yet

    ws.collect_diags();
    EXPECT_TRUE(ws.diags().empty());

    parse_all_files(ws);
    ws.collect_diags();
    EXPECT_FALSE(ws.diags().empty());
}
