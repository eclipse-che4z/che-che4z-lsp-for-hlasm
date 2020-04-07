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

#include "gtest/gtest.h"

#include "workspaces/workspace.h"
#include "workspaces/file_manager_impl.h"
#include "workspaces/file_impl.h"

#include <iterator>
#include <algorithm>
#include <fstream>

using namespace hlasm_plugin::parser_library;
using namespace hlasm_plugin::parser_library::workspaces;

class workspace_test : public diagnosable_impl, public testing::Test
{
public:
	void collect_diags() const override {}
	size_t collect_and_get_diags_size(workspace & ws, file_manager & file_mngr)
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
	file_manager_impl file_mngr;

#if _WIN32
	workspace ws("test\\library\\test_wks", file_mngr);
#else
	workspace ws("test/library/test_wks", file_mngr);
#endif
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

	ws.parse_library("MACRO1", ctx_1, library_data{ processing::processing_kind::MACRO,ctx_1.ids().add("MACRO1") });

	//test, that macro1 is parsed, once we are able to parse macros (mby in ctx)

	collect_diags_from_child(ws);
	EXPECT_EQ(diags().size(), (size_t) 0);

	ws.parse_library("not_existing", ctx_2, library_data{ processing::processing_kind::MACRO,ctx_1.ids().add("not_existing") });

}

class file_proc_grps : public file_impl
{
public:
	file_proc_grps() :file_impl("proc_grps.json") {}

	file_uri uri = "test_uri";

	virtual const file_uri & get_file_name() override
	{
		return uri;
	}

	virtual const std::string & get_text() override
	{
		return file;
	}

	virtual bool update_and_get_bad() override
	{
		return false;
	}

#ifdef _WIN32
	std::string file = R"({
  "pgroups": [
    {
      "name": "P1",
      "libs": [
        "C:\\Users\\Desktop\\ASLib",
        "lib",
        "libs\\lib2\\",
		"",
        {
          "run": "ftp \\192.168.12.145\\MyASLib",
          "list": "",
          "cache_expire":"",
          "location": "downLibs"
        }
      ]
    },
    {
      "name": "P2",
      "libs": [
        "C:\\Users\\Desktop\\ASLib",
        "P2lib",
        "P2libs\\libb",
        {
          "run": "ftp \\192.168.12.145\\MyASLib",
          "location": "\\downLibs"
        }
      ]
    }
  ]
})";
#else
	std::string file = R"({
		"pgroups": [
			{
				"name": "P1",
				"libs": [
					"/home/user/ASLib",
					"lib",
					"libs/lib2/",
			"",
					{
						"run": "ftp /192.168.12.145/MyASLib",
						"list": "",
						"cache_expire":"",
						"location": "downLibs"
					}
				]
			},
			{
				"name": "P2",
				"libs": [
					"/home/user/ASLib",
					"P2lib",
					"P2libs/libb",
					{
						"run": "ftp /192.168.12.145/MyASLib",
						"location": "downLibs"
					}
				]
			}
		]
	})";
#endif //_WIN32
};

class file_pgm_conf : public file_impl
{
public:
	file_pgm_conf() :file_impl("proc_grps.json") {}

	file_uri uri = "test_uri";

	virtual const file_uri & get_file_name() override
	{
		return uri;
	}

	virtual const std::string & get_text() override
	{
		return file;
	}

	virtual bool update_and_get_bad() override
	{
		return false;
	}

#if _WIN32
	std::string file = R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgms\\*",
      "pgroup": "P2"
    }
  ]
})";
#else
	std::string file = R"({
  "pgms": [
    {
      "program": "pgm1",
      "pgroup": "P1"
    },
    {
      "program": "pgms/*",
      "pgroup": "P2"
    }
  ]
})";
#endif
};

class file_manager_proc_grps_test : public file_manager_impl
{

public:
	file_ptr add_file(const file_uri & uri) override
	{
		if (uri.substr(uri.size() - 14) == "proc_grps.json")
			return proc_grps;
		else
			return pgm_conf;
	}

	std::shared_ptr <file_proc_grps> proc_grps = std::make_shared<file_proc_grps>();
	std::shared_ptr <file_pgm_conf> pgm_conf = std::make_shared<file_pgm_conf>();


	// Inherited via file_manager
	virtual void did_open_file(const std::string &, version_t, std::string) override {}
	virtual void did_change_file(const std::string &, version_t, const document_change *, size_t) override {}
	virtual void did_close_file(const std::string &) override {}
};

TEST(workspace, load_config_synthetic)
{
	file_manager_proc_grps_test file_manager;
	workspace ws("test_proc_grps_uri", "test_proc_grps_name", file_manager);

	ws.open();

	auto & pg = ws.get_proc_grp("P1");
	EXPECT_EQ("P1", pg.name());
#ifdef _WIN32
	std::string expected[4]{ "C:\\Users\\Desktop\\ASLib\\", "test_proc_grps_uri\\lib\\", "test_proc_grps_uri\\libs\\lib2\\", "test_proc_grps_uri\\" };
#else
	std::string expected[4]{ "/home/user/ASLib/", "test_proc_grps_uri/lib/", "test_proc_grps_uri/libs/lib2/", "test_proc_grps_uri/" };
#endif // _WIN32
	EXPECT_EQ(std::size(expected), pg.libraries().size());
	for (size_t i = 0; i < std::min(std::size(expected), pg.libraries().size()); ++i)
	{
		library_local * libl = dynamic_cast<library_local *>(pg.libraries()[i].get());
		ASSERT_NE(libl, nullptr);
		EXPECT_EQ(expected[i], libl->get_lib_path());
	}

	auto & pg2 = ws.get_proc_grp("P2");
	EXPECT_EQ("P2", pg2.name()); 
#ifdef _WIN32
	std::string expected2[3]{ "C:\\Users\\Desktop\\ASLib\\", "test_proc_grps_uri\\P2lib\\", "test_proc_grps_uri\\P2libs\\libb\\" };
#else
	std::string expected2[3]{ "/home/user/ASLib/", "test_proc_grps_uri/P2lib/", "test_proc_grps_uri/P2libs/libb/" };
#endif // _WIN32
	EXPECT_EQ(std::size(expected2), pg2.libraries().size());
	for (size_t i = 0; i < std::min(std::size(expected2), pg2.libraries().size()); ++i)
	{
		library_local * libl = dynamic_cast<library_local *>(pg2.libraries()[i].get());
		ASSERT_NE(libl, nullptr);
		EXPECT_EQ(expected2[i], libl->get_lib_path());
	}


	//test of pgm_conf and workspace::get_proc_grp_by_program
	#ifdef _WIN32
		auto & pg3 = ws.get_proc_grp_by_program("test_proc_grps_uri\\pgm1");
	#else
		auto & pg3 = ws.get_proc_grp_by_program("test_proc_grps_uri/pgm1");
	#endif
	EXPECT_EQ(pg3.libraries().size(), std::size(expected));
	for (size_t i = 0; i < std::min(std::size(expected), pg3.libraries().size()); ++i)
	{
		library_local * libl = dynamic_cast<library_local *>(pg3.libraries()[i].get());
		ASSERT_NE(libl, nullptr);
		EXPECT_EQ(expected[i], libl->get_lib_path());
	}

	//test of 
	#ifdef _WIN32
		auto & pg4 = ws.get_proc_grp_by_program("test_proc_grps_uri\\pgms\\anything");
	#else
			auto & pg4 = ws.get_proc_grp_by_program("test_proc_grps_uri/pgms/anything");
	#endif
	EXPECT_EQ(pg4.libraries().size(), std::size(expected2));
	for (size_t i = 0; i < std::min(std::size(expected2), pg4.libraries().size()); ++i)
	{
		library_local * libl = dynamic_cast<library_local *>(pg4.libraries()[i].get());
		ASSERT_NE(libl, nullptr);
		EXPECT_EQ(expected2[i], libl->get_lib_path());
	}
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
	file_with_text(const std::string & name, const std::string & text) : file_impl(name),processor_file_impl(name)
	{
		did_open(text, 1);
	}

	virtual const std::string& get_text() override
	{
		return get_text_ref();
	}

	virtual bool update_and_get_bad() override
	{
		return false;
	}
};

#ifdef _WIN32
constexpr const char * faulty_macro_path = "lib\\ERROR";
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
		files_.emplace(hlasmplugin_folder + "proc_grps.json", std::make_unique<file_with_text>("proc_grps.json", pgroups_file));
		files_.emplace(hlasmplugin_folder + "pgm_conf.json", std::make_unique<file_with_text>("pgm_conf.json", pgmconf_file));
		files_.emplace("source1", std::make_unique<file_with_text>("source1", source_using_macro_file));
		files_.emplace("source2", std::make_unique<file_with_text>("source2", source_using_macro_file));
		files_.emplace("source3", std::make_unique<file_with_text>("source3", source_using_macro_file_no_error));
		files_.emplace(faulty_macro_path, std::make_unique<file_with_text>(faulty_macro_path, faulty_macro_file));
		files_.emplace(correct_macro_path, std::make_unique<file_with_text>(correct_macro_path, correct_macro_file));
	}

	virtual std::unordered_map<std::string,std::string> list_directory_files(const std::string &) override
	{	
		if (insert_correct_macro)
			return { {"ERROR","ERROR"}, {"CORRECT","CORRECT"} };
		return { {"ERROR","ERROR" } };
	}

	bool insert_correct_macro = true;
};



TEST_F(workspace_test, did_close_file)
{
	file_manager_extended file_manager;
	workspace ws("", "workspace_name", file_manager);
	ws.open();
	// 3 files are open
	//	- open codes source1 and source2 with syntax errors using macro ERROR
	//	- macro file lib/ERROR with syntax error
	// on first reparse, there should be 3 diagnotics from sources and lib/ERROR file
	ws.did_open_file("source1");
	ws.did_open_file("source2");
	ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)3);
	EXPECT_TRUE(match_strings({ faulty_macro_path,"source2","source1" }));

	// when we close source1, only its diagnostics should disapear
	// macro's and source2's diagnostics should stay as it is still open
	ws.did_close_file("source1");
	ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
	EXPECT_TRUE(match_strings({ faulty_macro_path,"source2" }));

	// even though we close the ERROR macro, its diagnostics will still be there as it is a dependency of source2
	ws.did_close_file(faulty_macro_path);
	ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)2);
	EXPECT_TRUE(match_strings({ faulty_macro_path,"source2" }));

	// if we remove the line using ERROR macro in the source2. its diagnostics will be removed as it is no longer a dependendancy of source2
	std::vector<document_change> changes;
	std::string new_text = "";
	changes.push_back(document_change({ {0, 0}, {0, 6} }, new_text.c_str(), new_text.size()));
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
	workspace ws("", "workspace_name", file_manager);
	ws.open();

	// no diagnostics with no syntax errors
	ws.did_open_file("source3");
	EXPECT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);

	// remove the macro, there should still be 1 diagnostic E049 that the ERROR was not found
	file_manager.insert_correct_macro = false;
	ws.did_change_watched_files(correct_macro_path);
	ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)1);
	EXPECT_STREQ(diags()[0].code.c_str(), "E049");

	//put it back and make some change in the source file, the diagnostic will disappear
	file_manager.insert_correct_macro = true;
	ws.did_change_watched_files(correct_macro_path);
	std::vector<document_change> changes;
	std::string new_text = "";
	changes.push_back(document_change({ {0, 0}, {0, 0} }, new_text.c_str(), new_text.size()));
	ws.did_change_file("source3", changes.data(), changes.size());
	ASSERT_EQ(collect_and_get_diags_size(ws, file_manager), (size_t)0);
}
