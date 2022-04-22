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
#include <fstream>
#include <iterator>

#include "gtest/gtest.h"

#include "../common_testing.h"
#include "file_with_text.h"
#include "utils/path.h"
#include "utils/platform.h"
#include "workspaces/file_impl.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using hlasm_plugin::utils::platform::is_windows;

class workspace_instruction_sets_test : public diagnosable_impl, public testing::Test
{
public:
    void collect_diags() const override {}
    size_t collect_and_get_diags_size(workspace& ws, file_manager& file_mngr)
    {
        diags().clear();
        collect_diags_from_child(ws);
        collect_diags_from_child(file_mngr);
        return diags().size();
    }
};

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

const char* sam31_macro_path = is_windows() ? "lib\\SAM31" : "lib/SAM31";
std::string hlasmplugin_folder = is_windows() ? ".hlasmplugin\\" : ".hlasmplugin/";

enum class file_manager_opt_variant
{
    optable_370,
    optable_Z10
};

class file_manager_opt : public file_manager_impl
{
    std::unique_ptr<file_with_text> generate_proc_grps_file(file_manager_opt_variant variant)
    {
        switch (variant)
        {
            case file_manager_opt_variant::optable_370:
                return std::make_unique<file_with_text>("proc_grps.json", pgroups_file_optable_370, *this);
            case file_manager_opt_variant::optable_Z10:
                return std::make_unique<file_with_text>("proc_grps.json", pgroups_file_optable_Z10, *this);
        }
        throw std::logic_error("Not implemented");
    }

public:
    file_manager_opt(file_manager_opt_variant variant)
    {
        files_.emplace(hlasmplugin_folder + "proc_grps.json", generate_proc_grps_file(variant));
        files_.emplace(hlasmplugin_folder + "pgm_conf.json",
            std::make_unique<file_with_text>("pgm_conf.json", pgmconf_file, *this));
        files_.emplace("source", std::make_unique<file_with_text>("source", source, *this));
        files_.emplace(sam31_macro_path, std::make_unique<file_with_text>(sam31_macro_path, sam31_macro, *this));
    }

    list_directory_result list_directory_files(const std::string& path) override
    {
        if (path == "lib/" || path == "lib\\")
            return { { { "SAM31", "SAM31" } }, hlasm_plugin::utils::path::list_directory_rc::done };

        return { {}, hlasm_plugin::utils::path::list_directory_rc::not_exists };
    }
};

void change_instruction_set(
    const range& change_range, const std::string& process_group, file_manager& fm, workspace& ws)
{
    std::vector<document_change> changes;
    changes.push_back(document_change({ change_range }, process_group.c_str(), process_group.size()));

    fm.did_change_file(hlasmplugin_folder + "proc_grps.json", 1, changes.data(), changes.size());
    ws.did_change_file(hlasmplugin_folder + "proc_grps.json", changes.data(), changes.size());
}
} // namespace

TEST_F(workspace_instruction_sets_test, changed_instr_set_370_Z10)
{
    file_manager_opt file_manager(file_manager_opt_variant::optable_370);
    lib_config config;
    workspace ws("", "workspace_name", file_manager, config);
    ws.open();

    ws.did_open_file("source");
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);

    // Change instruction set
    change_instruction_set({ { 0, 0 }, { 12, 1 } }, pgroups_file_optable_Z10, file_manager, ws);

    collect_and_get_diags_size(ws, file_manager);
    EXPECT_TRUE(matches_message_codes(diags(), { "E049" }));
}

TEST_F(workspace_instruction_sets_test, changed_instr_set_Z10_370)
{
    file_manager_opt file_manager(file_manager_opt_variant::optable_Z10);
    lib_config config;
    workspace ws("", "workspace_name", file_manager, config);
    ws.open();

    ws.did_open_file("source");
    collect_and_get_diags_size(ws, file_manager);
    EXPECT_TRUE(matches_message_codes(diags(), { "E049" }));

    // Change instruction set
    change_instruction_set({ { 0, 0 }, { 12, 1 } }, pgroups_file_optable_370, file_manager, ws);

    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}