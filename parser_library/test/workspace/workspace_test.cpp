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
#include "external_configuration_requests_mock.h"
#include "external_file_reader_mock.h"
#include "utils/platform.h"
#include "utils/resource_location.h"
#include "workspaces/file_manager.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"
#include "workspaces/workspace_configuration.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;
using namespace hlasm_plugin::utils::resource;
using namespace hlasm_plugin::utils;
using hlasm_plugin::utils::platform::is_windows;
using namespace ::testing;

namespace {
const auto correct_loc = is_windows() ? resource_location("test\\library\\test_wks\\correct")
                                      : resource_location("test/library/test_wks/correct");

std::vector<diagnostic> extract_diags(workspace& ws, workspace_configuration& cfg)
{
    std::vector<diagnostic> result;
    cfg.produce_diagnostics(result, {}, {});
    ws.produce_diagnostics(result);
    return result;
}

bool match_file_uri(const std::vector<diagnostic>& diags, std::initializer_list<resource_location> set)
{
    return matches_message_properties(diags, set, &diagnostic::file_uri, &resource_location::get_uri);
}

} // namespace

class workspace_test : public testing::Test
{
public:
    lib_config config;
    shared_json global_settings = make_empty_shared_json();
};

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
    },
    {
      "program": "source4",
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

std::string cordep_macro_file = R"( MACRO
 CORDEP
 COPY DEP
 MEND
)";

std::string dep_macro_file = R"(
 MNOTE 'DEP'
)";

std::string source_using_macro_file = R"( ERROR
label
)";

std::string source_using_macro_file_no_error = R"( CORRECT)";

std::string source_using_macro_with_dep = R"( CORDEP)";

const resource_location ws_loc("ws:/");
const resource_location lib_loc("ws:/lib/");

const resource_location proc_grps_loc("ws:/.hlasmplugin/proc_grps.json");
const resource_location pgm_conf_loc("ws:/.hlasmplugin/pgm_conf.json");
const resource_location source1_loc("ws:/source1");
const resource_location source2_loc("ws:/source2");
const resource_location source3_loc("ws:/source3");
const resource_location source4_loc("ws:/source4");
const resource_location faulty_macro_loc("ws:/lib/ERROR");
const resource_location correct_macro_loc("ws:/lib/CORRECT");
const resource_location cordep_macro_loc("ws:/lib/CORDEP");
const resource_location dep_macro_loc("ws:/lib/DEP");
} // namespace

class file_manager_extended : public file_manager_impl, public external_file_reader
{
    std::map<resource_location, std::string> file_contents = {
        { proc_grps_loc, pgroups_file },
        { pgm_conf_loc, pgmconf_file },
        { source1_loc, source_using_macro_file },
        { source2_loc, source_using_macro_file },
        { source3_loc, source_using_macro_file_no_error },
        { source4_loc, source_using_macro_with_dep },
        { faulty_macro_loc, faulty_macro_file },
        { correct_macro_loc, correct_macro_file },
        { cordep_macro_loc, cordep_macro_file },
        { dep_macro_loc, dep_macro_file },
    };

public:
    file_manager_extended()
        : file_manager_impl(static_cast<external_file_reader&>(*this))
    {
        for (const auto& [loc, content] : file_contents)
            did_open_file(loc, 1, content);
    }

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(
        const hlasm_plugin::utils::resource::resource_location&) const override
    {
        if (insert_correct_macro)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
                {
                    { "ERROR", faulty_macro_loc },
                    { "CORRECT", correct_macro_loc },
                    { "CORDEP", cordep_macro_loc },
                    { "DEP", dep_macro_loc },
                },
                hlasm_plugin::utils::path::list_directory_rc::done,
            });
        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {
                { "ERROR", faulty_macro_loc },
                { "CORDEP", cordep_macro_loc },
                { "DEP", dep_macro_loc },
            },
            hlasm_plugin::utils::path::list_directory_rc::done,
        });
    }


    hlasm_plugin::utils::value_task<list_directory_result> list_directory_subdirs_and_symlinks(
        const hlasm_plugin::utils::resource::resource_location&) const override
    {
        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {},
            hlasm_plugin::utils::path::list_directory_rc::other_failure,
        });
    }


    hlasm_plugin::utils::value_task<std::optional<std::string>> load_text(
        const resource_location& document_loc) const override
    {
        if (auto it = file_contents.find(document_loc); it != file_contents.end())
            return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(it->second);
        return hlasm_plugin::utils::value_task<std::optional<std::string>>::from_value(std::nullopt);
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

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == lib_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
                {
                    { "CORRECT", correct_macro_loc },
                },
                hlasm_plugin::utils::path::list_directory_rc::done,
            });

        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {},
            hlasm_plugin::utils::path::list_directory_rc::not_exists,
        });
    }
};


TEST_F(workspace_test, did_close_file)
{
    file_manager_extended file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);

    ws_cfg.parse_configuration_file().run();
    // 3 files are open
    // - open codes source1 and source2 with syntax errors using macro ERROR
    // - macro file lib/ERROR with syntax error
    // on first reparse, there should be 3 diagnostics from sources and lib/ERROR file
    run_if_valid(ws.did_open_file(source1_loc));
    run_if_valid(ws.did_open_file(source2_loc));
    parse_all_files(ws);
    EXPECT_TRUE(match_file_uri(extract_diags(ws, ws_cfg), { faulty_macro_loc, source2_loc, source1_loc }));

    // when we close source1, only its diagnostics should disappear
    // macro's and source2's diagnostics should stay as it is still open
    run_if_valid(ws.did_close_file(source1_loc));
    parse_all_files(ws);
    EXPECT_TRUE(match_file_uri(extract_diags(ws, ws_cfg), { faulty_macro_loc, source2_loc }));

    // even though we close the ERROR macro, its diagnostics will still be there as it is a dependency of source2
    run_if_valid(ws.did_close_file(faulty_macro_loc));
    parse_all_files(ws);
    EXPECT_TRUE(match_file_uri(extract_diags(ws, ws_cfg), { faulty_macro_loc, source2_loc }));

    // if we remove the line using ERROR macro in the source2. its diagnostics will be removed as it is no longer a
    // dependency of source2
    std::vector<document_change> changes;
    std::string new_text = "";
    changes.push_back(document_change({ { 0, 0 }, { 0, 6 } }, new_text));
    file_manager.did_change_file(source2_loc, 1, changes);
    run_if_valid(ws.mark_file_for_parsing(source2_loc, file_content_state::changed_content));
    parse_all_files(ws);
    EXPECT_TRUE(match_file_uri(extract_diags(ws, ws_cfg), { source2_loc }));

    // finally if we close the last source2 file, its diagnostics will disappear as well
    run_if_valid(ws.did_close_file(source2_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, did_close_file_without_save)
{
    file_manager_extended file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);

    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source3_loc));
    run_if_valid(ws.did_open_file(correct_macro_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    document_change c(range(position(2, 0), position(2, 0)), "ERR\n");
    file_manager.did_change_file(correct_macro_loc, 2, std::span(&c, 1));
    run_if_valid(ws.mark_file_for_parsing(correct_macro_loc, file_content_state::changed_content));
    parse_all_files(ws);

    // TODO: This was modified due to very specific behavior of W010 diagnostic.
    // It is not associated with any statement, so it is only displayed
    // when a file is interpreted as an opencode.
    // This is part of the problem, where it is not clear what diagnostic to display
    // when a macro is being browsed with another opencode as a context.

    EXPECT_TRUE(match_file_uri(extract_diags(ws, ws_cfg), { correct_macro_loc }));

    file_manager.did_close_file(correct_macro_loc);
    run_if_valid(ws.did_close_file(correct_macro_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, did_change_watched_files)
{
    file_manager_extended file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source3_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    file_manager.insert_correct_macro = false;
    run_if_valid(ws_cfg.refresh_libraries({ correct_macro_loc }).then([&ws](auto r) {
        return ws.did_change_watched_files(
            { correct_macro_loc }, { workspaces::file_content_state::identical }, std::move(r));
    }));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "E049" }));

    file_manager.insert_correct_macro = true;
    run_if_valid(ws_cfg.refresh_libraries({ correct_macro_loc }).then([&ws](auto r) {
        return ws.did_change_watched_files(
            { correct_macro_loc }, { workspaces::file_content_state::identical }, std::move(r));
    }));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, did_change_watched_files_not_opened_file)
{
    file_manager_extended file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source3_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());

    run_if_valid(ws.did_change_watched_files({ faulty_macro_loc }, { workspaces::file_content_state::identical }, {}));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, diagnostics_recollection)
{
    file_manager_opt file_manager(file_manager_opt_variant::required);
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source1_loc));
    parse_all_files(ws);

    const auto original_diags_size = extract_diags(ws, ws_cfg).size();
    EXPECT_GE(original_diags_size, (size_t)1);

    EXPECT_EQ(extract_diags(ws, ws_cfg).size(), original_diags_size);
}

TEST_F(workspace_test, missing_library_required)
{
    for (auto type : { file_manager_opt_variant::old_school,
             file_manager_opt_variant::default_to_required,
             file_manager_opt_variant::required })
    {
        file_manager_opt file_manager(type);
        workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
        workspace ws(file_manager, ws_cfg);
        ws_cfg.parse_configuration_file().run();

        run_if_valid(ws.did_open_file(source1_loc));
        parse_all_files(ws);
        EXPECT_TRUE(contains_message_codes(extract_diags(ws, ws_cfg), { "L0002" }));
    }
}

TEST_F(workspace_test, missing_library_optional)
{
    file_manager_opt file_manager(file_manager_opt_variant::optional);
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source1_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, invalid_assembler_options)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_assembler_options);
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(contains_message_codes(extract_diags(ws, ws_cfg), { "W0005" }));
}

TEST_F(workspace_test, invalid_assembler_options_in_pgm_conf)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_assembler_options_in_pgm_conf);
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(contains_message_codes(extract_diags(ws, ws_cfg), { "W0005" }));
}

TEST_F(workspace_test, invalid_preprocessor_options)
{
    file_manager_opt file_manager(file_manager_opt_variant::invalid_preprocessor_options);
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_TRUE(contains_message_codes(extract_diags(ws, ws_cfg), { "W0006" }));
}

class file_manager_list_dir_failed : public file_manager_opt
{
public:
    file_manager_list_dir_failed()
        : file_manager_opt(file_manager_opt_variant::old_school)
    {}

    hlasm_plugin::utils::value_task<list_directory_result> list_directory_files(
        const hlasm_plugin::utils::resource::resource_location& location) const override
    {
        if (location == lib_loc)
            return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
                {},
                hlasm_plugin::utils::path::list_directory_rc::other_failure,
            });

        return hlasm_plugin::utils::value_task<list_directory_result>::from_value({
            {},
            hlasm_plugin::utils::path::list_directory_rc::not_exists,
        });
    }
};

TEST_F(workspace_test, library_list_failure)
{
    file_manager_list_dir_failed file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    run_if_valid(ws.did_open_file(source1_loc));
    parse_all_files(ws);
    EXPECT_TRUE(contains_message_codes(extract_diags(ws, ws_cfg), { "L0001" }));
}

TEST_F(workspace_test, did_change_watched_files_added_missing)
{
    file_manager_extended file_manager;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    file_manager.insert_correct_macro = false;
    run_if_valid(ws.did_open_file(source3_loc));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "E049" }));

    file_manager.insert_correct_macro = true;
    run_if_valid(ws_cfg.refresh_libraries({ correct_macro_loc }).then([&ws](auto r) {
        return ws.did_change_watched_files(
            { correct_macro_loc }, { workspaces::file_content_state::identical }, std::move(r));
    }));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, use_external_library)
{
    NiceMock<external_configuration_requests_mock> external_conf_mock;
    NiceMock<external_file_reader_mock> external_files;
    file_manager_impl fm(external_files);

    const auto source_match = [](auto seq) { return seq == source1_loc.get_uri(); };
    const auto provide_pg = [](auto, auto channel) {
        channel.provide(R"(
    {
      "name": "P1",
      "libs": [ {"dataset": "REMOTE.DATASET"} ]
    })");
    };
    EXPECT_CALL(external_conf_mock, read_external_configuration(Truly(source_match), _)).WillOnce(Invoke(provide_pg));
    fm.did_open_file(source1_loc, 1, "");

    workspace_configuration ws_cfg(fm, resource_location(), global_settings, config, &external_conf_mock, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_CALL(external_files, list_directory_files(resource_location("hlasm-external:/DATASET/REMOTE.DATASET")))
        .WillOnce(Invoke(
            []() { return value_task<list_directory_result>::from_value({ {}, path::list_directory_rc::done }); }));


    run_if_valid(ws.did_open_file(source1_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, use_external_library_with_workspace_uri)
{
    NiceMock<external_file_reader_mock> external_files;
    file_manager_impl fm(external_files);

    fm.did_open_file(pgm_conf_loc, 1, R"({
  "pgms": [
    {
      "program": "source1",
      "pgroup": "P1"
    }
  ]
})");
    fm.did_open_file(proc_grps_loc, 1, R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ {"dataset": "REMOTE.DATASET"} ]
    }
  ]
})");
    fm.did_open_file(source1_loc, 1, "");

    workspace_configuration ws_cfg(fm, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(fm, ws_cfg);
    ws_cfg.parse_configuration_file().run();

    EXPECT_CALL(external_files, list_directory_files(resource_location("hlasm-external:/DATASET/REMOTE.DATASET")))
        .WillOnce(Invoke(
            []() { return value_task<list_directory_result>::from_value({ {}, path::list_directory_rc::done }); }));


    run_if_valid(ws.did_open_file(source1_loc));
    parse_all_files(ws);
    EXPECT_TRUE(extract_diags(ws, ws_cfg).empty());
}

TEST_F(workspace_test, track_nested_dependencies)
{
    file_manager_extended file_manager;
    config.diag_supress_limit = 0;
    workspace_configuration ws_cfg(file_manager, ws_loc, global_settings, config, nullptr, nullptr);
    workspace ws(file_manager, ws_cfg);

    ws_cfg.parse_configuration_file().run();
    run_if_valid(ws.did_open_file(source4_loc));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "MNOTE" }));

    run_if_valid(ws.did_open_file(dep_macro_loc, file_content_state::changed_lsp));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "MNOTE" }));

    run_if_valid(ws.mark_file_for_parsing(source4_loc, file_content_state::changed_content));
    parse_all_files(ws);
    EXPECT_TRUE(matches_message_codes(extract_diags(ws, ws_cfg), { "MNOTE" }));
}
