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

#include "lib_config.h"
#include "message_consumer_mock.h"
#include "nlohmann/json.hpp"
#include "utils/path.h"
#include "utils/platform.h"
#include "workspace_manager.h"

using namespace hlasm_plugin::parser_library;

class diag_consumer_mock : public diagnostics_consumer
{
public:
    // Inherited via diagnostics_consumer
    void consume_diagnostics(diagnostic_list diagnostics, fade_message_list fade_messages) override
    {
        diags = diagnostics;
        fms = fade_messages;
    }

    diagnostic_list diags;
    fade_message_list fms;
};

TEST(workspace_manager, add_not_existing_workspace)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "not_existing");

    size_t count = ws_mngr.get_workspaces_count();
    EXPECT_EQ(count, (size_t)1);

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, add_existing_workspace)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "test/library/test_wks");

    size_t count = ws_mngr.get_workspaces_count();
    EXPECT_EQ(count, (size_t)1);

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, did_open_file)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "test/library/test_wks");

    std::string input_text = "label lr 1,2";
    ws_mngr.did_open_file("test/library/test_wks/some_file", 1, input_text.c_str(), input_text.size());

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, did_change_file)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "test/library/test_wks");
    std::string input = "label lr 1,2 remark";
    ws_mngr.did_open_file("test/library/test_wks/new_file", 1, input.c_str(), input.size());

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)0);

    std::vector<document_change> changes;
    std::string new_text = "anop";
    changes.push_back(document_change({ { 0, 6 }, { 0, input.size() } }, new_text.c_str(), new_text.size()));

    ws_mngr.did_change_file("test/library/test_wks/new_file", 2, changes.data(), 1);

    EXPECT_EQ(consumer.diags.diagnostics_size(), (size_t)1);

    std::vector<document_change> changes1;
    std::string new_text1 = "";
    changes1.push_back(document_change({ { 0, 6 }, { 0, 10 } }, new_text1.c_str(), new_text1.size()));

    ws_mngr.did_change_file("test/library/test_wks/new_file", 3, changes1.data(), 1);

    EXPECT_GT(consumer.diags.diagnostics_size(), (size_t)0);
}

TEST(workspace_manager, fade_msgs)
{
    workspace_manager ws_mngr;
    diag_consumer_mock consumer;
    ws_mngr.register_diagnostics_consumer(&consumer);

    ws_mngr.add_workspace("workspace", "test/library/test_wks");
    std::string pgm_conf = R"({
  "pgms": [
    {
      "program": "file*",
      "pgroup": "P1"
    }
  ]
})";

    std::string proc_grps = R"({
  "pgroups": [
    {
      "name": "P1",
      "preprocessor":"DB2",
      "libs": []
    }
  ]
})";
    std::string f1 = R"(START    DS    0C
 EXEC SQL INCLUDE SQLCA
 END)";

    ws_mngr.did_open_file("test/library/test_wks/.hlasmplugin/pgm_conf.json", 1, pgm_conf.c_str(), pgm_conf.size());
    ws_mngr.did_open_file("test/library/test_wks/.hlasmplugin/proc_grps.json", 1, proc_grps.c_str(), proc_grps.size());
    ws_mngr.did_open_file("test/library/test_wks/file_1", 1, f1.c_str(), f1.size());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(1, 0), position(1, 23)));

    std::vector<document_change> changes;
    ws_mngr.did_change_file("test/library/test_wks/file_1", 2, changes.data(), 0);
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(1, 0), position(1, 23)));

    std::string new_f1_text = " EXEC SQL INCLUDE SQLCA   ";
    changes.push_back(document_change({ { 1, 0 }, { 1, 23 } }, new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 3, changes.data(), 1);
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(1, 0), position(1, 26)));

    std::string f2 = "";
    ws_mngr.did_open_file("test/library/test_wks/file_2", 1, f2.c_str(), f2.size());
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    ASSERT_EQ(consumer.fms.size(), static_cast<size_t>(1));
    EXPECT_EQ(std::string(consumer.fms.message(0).file_uri()), "test/library/test_wks/file_1");
    EXPECT_EQ(consumer.fms.message(0).get_range(), range(position(1, 0), position(1, 26)));

    new_f1_text = " EXEC INCLUDE SQLCA";
    changes.clear();
    changes.push_back(document_change({ { 1, 0 }, { 1, 26 } }, new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 4, changes.data(), 1);
    EXPECT_GE(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    EXPECT_EQ(consumer.fms.size(), static_cast<size_t>(0));

    new_f1_text = "";
    changes.clear();
    changes.push_back(document_change(new_f1_text.c_str(), new_f1_text.size()));
    ws_mngr.did_change_file("test/library/test_wks/file_1", 5, changes.data(), 1);
    EXPECT_EQ(consumer.diags.diagnostics_size(), static_cast<size_t>(0));
    EXPECT_EQ(consumer.fms.size(), static_cast<size_t>(0));
}
