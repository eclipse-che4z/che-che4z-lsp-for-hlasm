/*
 * Copyright (c) 2021 Broadcom.
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

#include <sstream>

#include "gmock/gmock.h"

#include "../ws_mngr_mock.h"
#include "dap/dap_session_manager.h"
#include "workspace_manager.h"

using namespace hlasm_plugin;
using namespace hlasm_plugin::language_server;

namespace {
class stream_json_sink : public json_sink
{
    std::stringstream ss;

public:
    std::string take_output()
    {
        std::string result = ss.str();
        ss.clear();

        return result;
    }

    // Inherited via json_sink
    void write(const nlohmann::json& j) override { ss << j.dump(); }
    void write(nlohmann::json&& j) override { ss << j.dump(); }
};
} // namespace

TEST(dap_sessions, simple_start_stop)
{
    std::atomic<bool> term = false;
    test::ws_mngr_mock mock_ws;
    stream_json_sink session_out;
    dap::session session(term, mock_ws, session_out);

    EXPECT_TRUE(session.is_running());
    EXPECT_EQ(session.get_session_id(), "hlasm/dap_tunnel/0");

    auto session_matcher = session.get_message_matcher();
    EXPECT_TRUE(session_matcher(nlohmann::json { { "method", "hlasm/dap_tunnel/0" } }));
    EXPECT_FALSE(session_matcher(nlohmann::json { { "method", "hlasm/dap_tunnel" } }));
    EXPECT_FALSE(session_matcher(nlohmann::json { { "method", "hlasm/dap_tunnel/1" } }));
}

TEST(dap_sessions, session_manager)
{
    test::ws_mngr_mock mock_ws;
    stream_json_sink session_out;
    dap::session_manager sess_mgr(mock_ws, session_out);

    auto session_manager_matcher = sess_mgr.get_filtering_predicate();
    EXPECT_TRUE(session_manager_matcher(nlohmann::json { { "method", "hlasm/dap_tunnel" } }));
    EXPECT_TRUE(session_manager_matcher(nlohmann::json { { "method", "hlasm/dap_tunnel/0" } }));

    sess_mgr.write(nlohmann::json { { "method", "hlasm/dap_tunnel" }, { "params", { { "session_id", 1 } } } });

    EXPECT_EQ(sess_mgr.registered_sessions_count(), 1);
}
