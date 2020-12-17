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

#include "workspaces/file_impl.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/workspace.h"

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;

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

    bool match_strings(std::vector<std::string> set)
    {
        if (diags().size() != set.size())
            return false;
        for (const auto& diag : diags())
        {
            bool matched = false;
            for (const auto& str : set)
            {
                if (diag.file_name == str)
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
    lib_config config;
    file_manager_impl file_mngr;

    std::string test_wks_path = (std::filesystem::path("test") / "library" / "test_wks").string();


    workspace ws(test_wks_path, file_mngr, config);

    ws.open();

    collect_diags_from_child(ws);
    collect_diags_from_child(file_mngr);
    EXPECT_EQ(diags().size(), (size_t)0);

    file_mngr.add_processor_file("test\\library\\test_wks\\correct");

#if _WIN32
    ws.did_open_file("test\\library\\test_wks\\correct");
    context::hlasm_context ctx_1("test\\library\\test_wks\\correct");
    context::hlasm_context ctx_2("test\\library\\test_wks\\correct");
#else
    ws.did_open_file("test/library/test_wks/correct");
    context::hlasm_context ctx_1("test/library/test_wks/correct");
    context::hlasm_context ctx_2("test/library/test_wks/correct");
#endif

    collect_diags_from_child(file_mngr);
    EXPECT_EQ(diags().size(), (size_t)0);

    diags().clear();

    ws.parse_library("MACRO1", ctx_1, library_data { processing::processing_kind::MACRO, ctx_1.ids().add("MACRO1") });

    // test, that macro1 is parsed, once we are able to parse macros (mby in ctx)

    collect_diags_from_child(ws);
    EXPECT_EQ(diags().size(), (size_t)0);

    ws.parse_library(
        "not_existing", ctx_2, library_data { processing::processing_kind::MACRO, ctx_1.ids().add("not_existing") });
}



std::string pgroups_file = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [ "lib" ]
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

class file_with_text : public processor_file_impl
{
public:
    file_with_text(const std::string& name, const std::string& text)
        : file_impl(name)
        , processor_file_impl(name)
    {
        did_open(text, 1);
    }

    virtual const std::string& get_text() override { return get_text_ref(); }

    virtual bool update_and_get_bad() override { return false; }
};

#ifdef _WIN32
constexpr const char* faulty_macro_path = "lib\\ERROR";
constexpr const char* correct_macro_path = "lib\\CORRECT";
std::string hlasmplugin_folder = ".hlasmplugin\\";
#else
constexpr const char* faulty_macro_path = "lib/ERROR";
constexpr const char* correct_macro_path = "lib/CORRECT";
std::string hlasmplugin_folder = ".hlasmplugin/";
#endif // _WIN32

class file_manager_extended : public file_manager_impl
{
public:
    file_manager_extended()
    {
        files_.emplace(
            hlasmplugin_folder + "proc_grps.json", std::make_unique<file_with_text>("proc_grps.json", pgroups_file));
        files_.emplace(
            hlasmplugin_folder + "pgm_conf.json", std::make_unique<file_with_text>("pgm_conf.json", pgmconf_file));
        files_.emplace("source1", std::make_unique<file_with_text>("source1", source_using_macro_file));
        files_.emplace("source2", std::make_unique<file_with_text>("source2", source_using_macro_file));
        files_.emplace("source3", std::make_unique<file_with_text>("source3", source_using_macro_file_no_error));
        files_.emplace(faulty_macro_path, std::make_unique<file_with_text>(faulty_macro_path, faulty_macro_file));
        files_.emplace(correct_macro_path, std::make_unique<file_with_text>(correct_macro_path, correct_macro_file));
    }

    virtual std::unordered_map<std::string, std::string> list_directory_files(const std::string&) override
    {
        if (insert_correct_macro)
            return { { "ERROR", "ERROR" }, { "CORRECT", "CORRECT" } };
        return { { "ERROR", "ERROR" } };
    }

    bool insert_correct_macro = true;
};



TEST_F(workspace_test, did_close_file)
{
    lib_config config;
    file_manager_extended file_manager;
    workspace ws("", "workspace_name", file_manager, config);
    ws.open();
    // 3 files are open
    //	- open codes source1 and source2 with syntax errors using macro ERROR
    //	- macro file lib/ERROR with syntax error
    // on first reparse, there should be 3 diagnotics from sources and lib/ERROR file
    ws.did_open_file("source1");
    ws.did_open_file("source2");
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)3);
    EXPECT_TRUE(match_strings({ faulty_macro_path, "source2", "source1" }));

    // when we close source1, only its diagnostics should disapear
    // macro's and source2's diagnostics should stay as it is still open
    ws.did_close_file("source1");
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
    EXPECT_TRUE(match_strings({ faulty_macro_path, "source2" }));

    // even though we close the ERROR macro, its diagnostics will still be there as it is a dependency of source2
    ws.did_close_file(faulty_macro_path);
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
    EXPECT_TRUE(match_strings({ faulty_macro_path, "source2" }));

    // if we remove the line using ERROR macro in the source2. its diagnostics will be removed as it is no longer a
    // dependendancy of source2
    std::vector<document_change> changes;
    std::string new_text = "";
    changes.push_back(document_change({ { 0, 0 }, { 0, 6 } }, new_text.c_str(), new_text.size()));
    file_manager.did_change_file("source2", 1, changes.data(), changes.size());
    ws.did_change_file("source2", changes.data(), changes.size());
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_TRUE(match_strings({ "source2" }));

    // finally if we close the last source2 file, its diagnostics will disappear as well
    ws.did_close_file("source2");
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}

TEST_F(workspace_test, did_change_watched_files)
{
    file_manager_extended file_manager;
    lib_config config;
    workspace ws("", "workspace_name", file_manager, config);
    ws.open();

    // no diagnostics with no syntax errors
    ws.did_open_file("source3");
    EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);

    // remove the macro, there should still be 1 diagnostic E049 that the ERROR was not found
    file_manager.insert_correct_macro = false;
    ws.did_change_watched_files(correct_macro_path);
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)1);
    EXPECT_STREQ(diags()[0].code.c_str(), "E049");

    // put it back and make some change in the source file, the diagnostic will disappear
    file_manager.insert_correct_macro = true;
    ws.did_change_watched_files(correct_macro_path);
    std::vector<document_change> changes;
    std::string new_text = "";
    changes.push_back(document_change({ { 0, 0 }, { 0, 0 } }, new_text.c_str(), new_text.size()));
    ws.did_change_file("source3", changes.data(), changes.size());
    ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}
