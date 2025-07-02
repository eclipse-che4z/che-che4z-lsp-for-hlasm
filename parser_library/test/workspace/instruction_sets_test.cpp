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

#include <algorithm>
#include <iterator>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "../workspace/empty_configs.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using hlasm_plugin::utils::platform::is_windows;

namespace {
std::string pgroups_file_optable_370 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ 
        "lib"
      ],
      "asm_options": {
        "OPTABLE": "370"
      }
    }
  ]
})";

std::string pgroups_file_optable_Z10 = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ 
        "lib"
      ],
      "asm_options": {
        "OPTABLE": "Z10"
      }
    }
  ]
})";

std::string pgmconf_file = R"({
  "pgms": [
    {
      "program": "source",
      "pgroup": "P1"
    }
  ]
})";

std::string source = R"(
        GBLA &VAR
&VAR    SETA   0
        SAM31

        AIF (&VAR EQ 2).END
        NONSENSE

.END    ANOP
        END)";

std::string sam31_macro = R"( MACRO
        SAM31
        GBLA &VAR
&VAR    SETA   2
        MEND)";

const resource_location ws_loc("ws:/");
const resource_location proc_grps_loc("ws:/.hlasmplugin/proc_grps.json");
const resource_location pgm_conf_loc("ws:/.hlasmplugin/pgm_conf.json");
const resource_location source_loc("ws:/source");
const resource_location sam31_macro_loc("ws:/lib/SAM31");

enum class file_manager_opt_variant
{
    optable_370,
    optable_Z10
};

class file_manager_opt : public file_manager_impl
{
    std::string get_proc_grp(file_manager_opt_variant variant)
    {
        switch (variant)
        {
            case file_manager_opt_variant::optable_370:
                return pgroups_file_optable_370;
            case file_manager_opt_variant::optable_Z10:
                return pgroups_file_optable_Z10;
        }
        throw std::logic_error("Not implemented");
    }

public:
    file_manager_opt(file_manager_opt_variant variant)
    {
        did_open_file(proc_grps_loc, 1, get_proc_grp(variant));
        did_open_file(pgm_conf_loc, 1, pgmconf_file);
        did_open_file(source_loc, 1, source);
        did_open_file(sam31_macro_loc, 1, sam31_macro);
    }

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        using hlasm_plugin::utils::value_task;
        if (location == resource_location("ws:/lib/"))
            return value_task<list_directory_result>::from_value({
                {
                    { "SAM31", sam31_macro_loc },
                },
                hlasm_plugin::utils::path::list_directory_rc::done,
            });

        return value_task<list_directory_result>::from_value({
            {},
            hlasm_plugin::utils::path::list_directory_rc::not_exists,
        });
    }
};

void change_instruction_set(const range& change_range,
    const std::string& process_group,
    file_manager& fm,
    workspace& ws,
    workspace_configuration& ws_cfg)
{
    std::vector<document_change> changes;
    changes.push_back(document_change({ change_range }, process_group));

    fm.did_change_file(proc_grps_loc, 1, changes);
    run_if_valid(ws_cfg.parse_configuration_file(proc_grps_loc).then([&ws](auto result) {
        if (result == parse_config_file_result::parsed)
            ws.mark_all_opened_files();
    }));
    parse_all_files(ws);
}

std::vector<diagnostic> extract_diags(workspace& ws, workspace_configuration& cfg)
{
    std::vector<diagnostic> result;
    cfg.produce_diagnostics(result, {}, {});
    ws.produce_diagnostics(result);
    return result;
}
} // namespace

TEST(workspace_instruction_sets_test, changed_instr_set_370_Z10)
{
    file_manager_opt file_manager(file_manager_opt_variant::optable_370);
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    // Change instruction set
    change_instruction_set({ { 0, 0 }, { 12, 1 } }, pgroups_file_optable_Z10, file_manager, ws, ws_cfg);

    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "E049" }));
}

TEST(workspace_instruction_sets_test, changed_instr_set_Z10_370)
{
    file_manager_opt file_manager(file_manager_opt_variant::optable_Z10);
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source_loc));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "E049" }));

    // Change instruction set
    change_instruction_set({ { 0, 0 }, { 12, 1 } }, pgroups_file_optable_370, file_manager, ws, ws_cfg);

    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}
