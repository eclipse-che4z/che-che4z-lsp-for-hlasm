
#include "shared/workspace_manager.h"

using namespace hlasm_plugin::parser_library;

class diag_consumer_mock : public diagnostics_consumer
{
public:
	// Inherited via diagnostics_consumer
	virtual void consume_diagnostics(diagnostic_list diagnostics) override
	{
		diags = diagnostics;
	}

	diagnostic_list diags;
};

TEST(workspace_manager, add_not_existing_workspace)
{
	workspace_manager ws_mngr;
	diag_consumer_mock consumer;
	ws_mngr.register_diagnostics_consumer(&consumer);

	ws_mngr.add_workspace("workspace", "not_existing");

	size_t count = ws_mngr.get_workspaces_count();
	EXPECT_EQ(count, (size_t) 1);

	EXPECT_GE(consumer.diags.diagnostics_size(), (size_t) 1);
}

TEST(workspace_manager, add_existing_workspace)
{
	workspace_manager ws_mngr;
	diag_consumer_mock consumer;
	ws_mngr.register_diagnostics_consumer(&consumer);

	ws_mngr.add_workspace("workspace", "test/library/test_wks");

	size_t count = ws_mngr.get_workspaces_count();
	EXPECT_EQ(count, (size_t) 1);

	EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, did_open_file)
{
	workspace_manager ws_mngr;
	diag_consumer_mock consumer;
	ws_mngr.register_diagnostics_consumer(&consumer);

	ws_mngr.add_workspace("workspace", "test/library/test_wks");

	ws_mngr.did_open_file("test/library/test_wks/", 1, "label lr 1,2", 12);

	EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);

}

TEST(workspace_manager, did_change_file)
{
	workspace_manager ws_mngr;
	diag_consumer_mock consumer;
	ws_mngr.register_diagnostics_consumer(&consumer);

	ws_mngr.add_workspace("workspace", "test\\library\\test_wks");

	ws_mngr.did_open_file("test\\library\\test_wks\\new_file", 1, "label lr 1,2 remark", 23);

	EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t) 0);

	std::vector<document_change> changes;
	std::string new_text = "anop";
	changes.push_back(document_change({ {0, 6}, {0, 23} }, new_text.c_str(), 4));

	ws_mngr.did_change_file("test\\library\\test_wks\\new_file", 2, changes.data(), 1);

	EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)1);

	std::vector<document_change> changes1;
	std::string new_text1 = "";
	changes1.push_back(document_change({ {0, 6}, {0, 10} }, new_text1.c_str(), 0));

	ws_mngr.did_change_file("test\\library\\test_wks\\new_file", 3, changes1.data(), 1);

	EXPECT_GT(consumer.diags.diagnostics_size(), (size_t) 0);

}