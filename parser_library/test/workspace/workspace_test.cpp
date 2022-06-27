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
#include <filesystem>
#include <fstream>
#include <iterator>

#include "gtest/gtest.h"

#include "empty_configs.h"
#include "file_with_text.h"
#include "utils/path.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_impl.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using hlasm_plugin::utils::platform::is_windows;

namespace {
const auto correct_loc = is_windows() ? resource_location("test\\library\\test_wks\\correct")
                                      : resource_location("test/library/test_wks/correct");
}

class workspace_test : public diagnosable_impl, public testing::Test
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

    bool match_strings(std::vector<resource_location> set)
    {
        if (diags().size() != set.size())
            return false;
        for (const auto& diag : diags())
        {
            bool matched = false;
            for (const auto& str : set)
            {
                if (diag.file_uri == str.get_uri())
                    matched = true;
            }
            if (!matched)
                return false;
        }
        return true;
    }
};

TEST_F(workspace_test, parse_lib_provider)
{
    using namespace hlasm_plugin::utils;

    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    file_manager_impl file_mngr;

    std::string test_wks_path = path::join(path::join("test", "library"), "test_wks").string();

    workspace ws(resource_location(test_wks_path), file_mngr, config, global_settings);

    ws.open();

    collect_diags_from_child(ws);
    collect_diags_from_child(file_mngr);
    EXPECT_EQ(diags().size(), (size_t)0);

    file_mngr.add_processor_file(correct_loc);

    auto [ctx_1, ctx_2] = [&ws]() {
        ws.did_open_file(correct_loc);
        return std::make_pair(std::make_shared<context::hlasm_context>(correct_loc),
            std::make_shared<context::hlasm_context>(correct_loc));
    }();

    collect_diags_from_child(file_mngr);
    EXPECT_EQ(diags().size(), (size_t)0);

    diags().clear();

    ws.parse_library("MACRO1",
        analyzing_context { ctx_1, std::make_shared<lsp::lsp_context>(ctx_1) },
        library_data { processing::processing_kind::MACRO, ctx_1->ids().add("MACRO1") });

    // test, that macro1 is parsed, once we are able to parse macros (mby in ctx)

    collect_diags_from_child(ws);
    EXPECT_EQ(diags().size(), (size_t)0);

    ws.parse_library("not_existing",
        analyzing_context { ctx_2, std::make_shared<lsp::lsp_context>(ctx_2) },
        library_data { processing::processing_kind::MACRO, ctx_2->ids().add("not_existing") });
}

namespace {
std::string pgroups_file = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "lib" ]
    }
  ]
})";

std::string pgroups_file_old_school = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "missing", "lib" ]
    }
  ]
})";

std::string pgroups_file_default = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        {
          "path": "missing"
        },
        "lib"
      ]
    }
  ]
})";

std::string pgroups_file_required = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        {
          "path": "missing",
          "optional": false
        },
        "lib"
      ]
    }
  ]
})";

std::string pgroups_file_optional = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        {
          "path": "missing",
          "optional": true
        },
        "lib"
      ]
    }
  ]
})";

std::string pgroups_file_invalid_assembler_options = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        {
          "path": "missing",
          "optional": true
        },
        "lib"
      ],
      "asm_options": {
        "SYSPARM": "AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJAAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJAAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJ"
      }
    }
  ]
})";

std::string pgroups_file_invalid_preprocessor_options = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        {
          "path": "missing",
          "optional": true
        },
        "lib"
      ],
      "preprocessor": {
        "name": "DB2",
        "options": {
          "version": "AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGG"
        }
      }
    }
  ]
})";

std::string pgmconf_file = R"({
  "pgms": [
    {
      "program": "source1",
      "pgroup": "P1"
    },
	{
      "program": "source2",
      "pgroup": "P1"
    },
	{
      "program": "source3",
      "pgroup": "P1"
    }
  ]
})";

std::string pgmconf_file_invalid_assembler_options = R"({
  "pgms": [
    {
      "program": "source1",
      "pgroup": "P1"
    },
	{
      "program": "source2",
      "pgroup": "P1"
    },
	{
      "program": "source3",
      "pgroup": "P1"
    },
	{
      "program": "invalid",
      "pgroup": "P1",
      "asm_options": {
        "SYSPARM": "AAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJAAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJAAAAAAAAAABBBBBBBBBBCCCCCCCCCCDDDDDDDDDDEEEEEEEEEEFFFFFFFFFFGGGGGGGGGGHHHHHHHHHHIIIIIIIIIIJJJJJJJJJJ"
      }
    }
  ]
})";

std::string faulty_macro_file = R"( MACRO
 ERROR
label
 MEND
)";

std::string correct_macro_file = R"( MACRO
 CORRECT
 MEND
)";

std::string source_using_macro_file = R"( ERROR
label
)";

std::string source_using_macro_file_no_error = R"( CORRECT)";

const char* faulty_macro_path = is_windows() ? "lib\\ERROR" : "lib/ERROR";
const char* correct_macro_path = is_windows() ? "lib\\CORRECT" : "lib/CORRECT";
const std::string hlasmplugin_folder = ".hlasmplugin";

const resource_location empty_loc = resource_location("");

const resource_location proc_grps_loc(hlasmplugin_folder + "/proc_grps.json");
const resource_location pgm_conf_loc(hlasmplugin_folder + "/pgm_conf.json");
const resource_location source1_loc("source1");
const resource_location source2_loc("source2");
const resource_location source3_loc("source3");
const resource_location faulty_macro_loc(faulty_macro_path);
const resource_location correct_macro_loc(correct_macro_path);
} // namespace

class file_manager_extended : public file_manager_impl
{
public:
    file_manager_extended()
    {
        did_open_file(proc_grps_loc, 1, pgroups_file);
        did_open_file(pgm_conf_loc, 1, pgmconf_file);
        did_open_file(source1_loc, 1, source_using_macro_file);
        did_open_file(source2_loc, 1, source_using_macro_file);
        did_open_file(source3_loc, 1, source_using_macro_file_no_error);
        did_open_file(faulty_macro_loc, 1, faulty_macro_file);
        did_open_file(correct_macro_loc, 1, correct_macro_file);
    }

    list_directory_result list_directory_files(const hlasm_plugin::utils::resource::resource_location&) const override
    {
        if (insert_correct_macro)
            return { { { "ERROR", faulty_macro_loc }, { "CORRECT", correct_macro_loc } },
                hlasm_plugin::utils::path::list_directory_rc::done };
        return { { { "ERROR", faulty_macro_loc } }, hlasm_plugin::utils::path::list_directory_rc::done };
    }

    bool insert_correct_macro = true;
};

enum class file_manager_opt_variant
{
    old_school,
    default_to_required,
    required,
    optional,
    invalid_assembler_options,
    invalid_preprocessor_options,
    invalid_assembler_options_in_pgm_conf,
};

class file_manager_opt : public file_manager_impl
{
    std::string generate_proc_grps_file(file_manager_opt_variant variant)
    {
        switch (variant)
        {
            case file_manager_opt_variant::old_school:
            case file_manager_opt_variant::invalid_assembler_options_in_pgm_conf:
                return pgroups_file_old_school;
            case file_manager_opt_variant::default_to_required:
                return pgroups_file_default;
            case file_manager_opt_variant::required:
                return pgroups_file_required;
            case file_manager_opt_variant::optional:
                return pgroups_file_optional;
            case file_manager_opt_variant::invalid_assembler_options:
                return pgroups_file_invalid_assembler_options;
            case file_manager_opt_variant::invalid_preprocessor_options:
                return pgroups_file_invalid_preprocessor_options;
        }
        throw std::logic_error("Not implemented");
    }

    std::string generate_pgm_conf_file(file_manager_opt_variant variant)
    {
        switch (variant)
        {
            case file_manager_opt_variant::invalid_assembler_options_in_pgm_conf:
                return pgmconf_file_invalid_assembler_options;
            default:
                return pgmconf_file;
        }
    }

public:
    file_manager_opt(file_manager_opt_variant variant)
    {
        did_open_file(proc_grps_loc, 1, generate_proc_grps_file(variant));
        did_open_file(pgm_conf_loc, 1, generate_pgm_conf_file(variant));
        did_open_file(source1_loc, 1, source_using_macro_file_no_error);
        did_open_file(correct_macro_loc, 1, correct_macro_file);
    }

    list_directory_result list_directory_files(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == resource_location("lib/"))
            return { { { "CORRECT", correct_macro_loc } }, hlasm_plugin::utils::path::list_directory_rc::done };

        return { {}, hlasm_plugin::utils::path::list_directory_rc::not_exists };
    }
};


TEST_F(workspace_test, did_close_file)
{
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    file_manager_extended file_manager;
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);

    ws.open();
    // 3 files are open
    //	- open codes source1 and source2 with syntax errors using macro ERROR
    //	- macro file lib/ERROR with syntax error
    // on first reparse, there should be 3 diagnostics from sources and lib/ERROR file
    ws.did_open_file(source1_loc);
    ws.did_open_file(source2_loc);
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)3);
    EXPECT_TRUE(match_strings({ faulty_macro_loc, source2_loc, source1_loc }));

    // when we close source1, only its diagnostics should disappear
    // macro's and source2's diagnostics should stay as it is still open
    ws.did_close_file(source1_loc);
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
    EXPECT_TRUE(match_strings({ faulty_macro_loc, source2_loc }));

    // even though we close the ERROR macro, its diagnostics will still be there as it is a dependency of source2
    ws.did_close_file(faulty_macro_loc);
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
    EXPECT_TRUE(match_strings({ faulty_macro_loc, source2_loc }));

    // if we remove the line using ERROR macro in the source2. its diagnostics will be removed as it is no longer a
    // dependency of source2
    std::vector<document_change> changes;
    std::string new_text = "";
    changes.push_back(document_change({ { 0, 0 }, { 0, 6 } }, new_text.c_str(), new_text.size()));
    file_manager.did_change_file(source2_loc, 1, changes.data(), changes.size());
    ws.did_change_file(source2_loc, changes.data(), changes.size());
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(match_strings({ source2_loc }));

    // finally if we close the last source2 file, its diagnostics will disappear as well
    ws.did_close_file(source2_loc);
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}

TEST_F(workspace_test, did_change_watched_files)
{
    file_manager_extended file_manager;
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    // no diagnostics with no syntax errors
    ws.did_open_file(source3_loc);
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);

    // remove the macro, there should still be 1 diagnostic E049 that the ERROR was not found
    file_manager.insert_correct_macro = false;
    ws.did_change_watched_files(correct_macro_loc);
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_STREQ(diags()[0].code.c_str(), "E049");

    // put it back and make some change in the source file, the diagnostic will disappear
    file_manager.insert_correct_macro = true;
    ws.did_change_watched_files(correct_macro_loc);
    std::vector<document_change> changes;
    std::string new_text = "";
    changes.push_back(document_change({ { 0, 0 }, { 0, 0 } }, new_text.c_str(), new_text.size()));
    ws.did_change_file(source3_loc, changes.data(), changes.size());
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}

TEST_F(workspace_test, missing_library_required)
{
    for (auto type : { file_manager_opt_variant::old_school,
             file_manager_opt_variant::default_to_required,
             file_manager_opt_variant::required })
    {
        file_manager_opt file_manager(type);
        lib_config config;
        workspace::shared_json global_settings = make_empty_shared_json();
        workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
        ws.open();

        ws.did_open_file(source1_loc);
        EXPECT_GE(collect_and_get_diags_size(ws, file_manager), (size_t)1);
        EXPECT_TRUE(std::any_of(diags().begin(), diags().end(), [](const auto& d) { return d.code == "L0002"; }));
    }
}

TEST_F(workspace_test, missing_library_optional)
{
    file_manager_opt file_manager(file_manager_opt_variant::optional);
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    ws.did_open_file(source1_loc);
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}

TEST_F(workspace_test, invalid_assembler_options)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_assembler_options);
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    EXPECT_GE(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(std::any_of(diags().begin(), diags().end(), [](const auto& d) { return d.code == "W0005"; }));
}

TEST_F(workspace_test, invalid_assembler_options_in_pgm_conf)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_assembler_options_in_pgm_conf);
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    EXPECT_GE(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(std::any_of(diags().begin(), diags().end(), [](const auto& d) { return d.code == "W0005"; }));
}

TEST_F(workspace_test, invalid_preprocessor_options)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_preprocessor_options);
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    EXPECT_GE(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(std::any_of(diags().begin(), diags().end(), [](const auto& d) { return d.code == "W0006"; }));
}

class file_manager_list_dir_failed : public file_manager_opt
{
public:
    file_manager_list_dir_failed()
        : file_manager_opt(file_manager_opt_variant::old_school)
    {}

    list_directory_result list_directory_files(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == resource_location("lib/"))
            return { {}, hlasm_plugin::utils::path::list_directory_rc::other_failure };

        return { {}, hlasm_plugin::utils::path::list_directory_rc::not_exists };
    }
};

TEST_F(workspace_test, library_list_failure)
{
    file_manager_list_dir_failed file_manager;
    lib_config config;
    workspace::shared_json global_settings = make_empty_shared_json();
    workspace ws(empty_loc, "workspace_name", file_manager, config, global_settings);
    ws.open();

    ws.did_open_file(source1_loc);
    EXPECT_GE(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(std::any_of(diags().begin(), diags().end(), [](const auto& d) { return d.code == "L0001"; }));
}